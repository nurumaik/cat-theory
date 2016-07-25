#include <iostream>
#include <cmath>
#include <utility>
#include <functional>
#include <random>
#include <cassert>
#include <unordered_map>
#include <memory>
#include <tuple>
#include <type_traits>

using std::forward;
using std::move;
using std::bind;
using std::cout;
using std::cerr;
using std::endl;
using std::make_unique;
using std::unordered_map;
using std::move;
using std::function;
using std::tuple;
using std::make_tuple;
namespace ph = std::placeholders;

// Identity and composition
// https://bartoszmilewski.com/2014/11/04/category-the-essence-of-composition/

/***
Can somebody explain me why this doesnt work?
auto id(auto x) {
    return forward<decltype(x)>(x);
}
***/

auto id = [](auto x) {
    return forward<decltype(x)>(x);
};

// * for composition

auto comp = [](auto f, auto g) {
    return [f, g](auto x) {
        return f ( g ( forward<decltype(x)> (x) ) );
    };
};

// Memoization
// https://bartoszmilewski.com/2014/11/24/types-and-functions/

// Custom hash to use tuples in unordered_map
// Proudly spiszheno from stackoverflow
namespace std{
    namespace
    {

        // Code from boost
        // Reciprocal of the golden ratio helps spread entropy
        //     and handles duplicates.
        // See Mike Seymour in magic-numbers-in-boosthash-combine:
        //     http://stackoverflow.com/questions/4948780

        template <class T>
        inline void hash_combine(std::size_t& seed, T const& v)
        {
            seed ^= hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }

        // Recursive template code derived from Matthieu M.
        template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
        struct HashValueImpl
        {
          static void apply(size_t& seed, Tuple const& tuple)
          {
            HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
            hash_combine(seed, get<Index>(tuple));
          }
        };

        template <class Tuple>
        struct HashValueImpl<Tuple,0>
        {
          static void apply(size_t& seed, Tuple const& tuple)
          {
            hash_combine(seed, get<0>(tuple));
          }
        };
    }

    template <typename ... TT>
    struct hash<std::tuple<TT...>> 
    {
        size_t
        operator()(std::tuple<TT...> const& tt) const
        {                                              
            size_t seed = 0;                             
            HashValueImpl<std::tuple<TT...> >::apply(seed, tt);    
            return seed;                                 
        }                                              

    };
}

template <typename F>
struct memoize;

// All Vs types needs to implement hash
// Also there can be some fuckup with forwards
template <typename U, typename... Vs>
struct memoize<U(Vs...)> {
    typedef 

    U operator()(Vs... x) {
        auto tupled_args = make_tuple(x...);
        auto memres = mem.find(tupled_args); 
        if (memres != mem.end()) {
            return memres->second;
        } else {
            auto res = func(forward<decltype(x)>(x)...);
            mem[tupled_args] = res;
            return res;
        }
    }

    memoize(function<U(Vs...)> f) : func(f) {}
private:
    function<U(Vs...)> func;
    unordered_map<tuple<Vs...>, U> mem;
};

// Kleisli categories
// https://bartoszmilewski.com/2014/12/23/kleisli-categories/

template<class A> 
class optional {
    bool _isValid;
    A    _value;
public:
    optional()    : _isValid(false) {}
    optional(A v) : _isValid(true), _value(v) {}
    bool isValid() const { return _isValid; }
    A value() const { assert(_isValid); return _value; }
};

optional<double> safe_root(double x) {
    if (x >= 0) return optional<double>{sqrt(x)};
    else return optional<double>{};
}

optional<double> safe_reciprocal(double x) {
    if (x == 0) {
        return {};
    } else {
        return {1./x};
    }
}

auto kleisli_compose = [](auto f, auto g) {
    return [f, g] (auto x) {
        auto y = g(forward<decltype(x)>(x));
        if (y.isValid()) {
            return f(move(y.value()));
        }
        return decltype(y){};
    };
};

auto safe_root_reciprocal = kleisli_compose(safe_root, safe_reciprocal);

// Some general functions to play with

int factorial(int n) {
    int result = 1;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }
    return result;
}

int add(int x, int y) {
    return x + y;
}


int main () {

    // Testing left and right identity laws
    const auto eps = .000001;
    auto f = [](auto x) {return sin(x);};
    for (auto t :{-3.14, -2.8, -1.2, 0., 1., 3.}) {
        auto only = f(t);
        auto left = comp(id, f)(t);
        auto right = comp(f, id)(t);
        assert(abs(only - left) < eps);
        assert(abs(only - right) < eps);
    }
    // Testing memoization
    auto fac = [](int n) -> int {return factorial(n);};
    auto memfac = memoize<decltype(factorial)>(fac);
    auto memadd = memoize<decltype(add)>(add);
    cout << memfac(5) << memadd(3, 5) << memadd(3, 5) << endl;

    // Optional chaining
    cout << safe_root_reciprocal(9).value() << endl;
    return 0;
}