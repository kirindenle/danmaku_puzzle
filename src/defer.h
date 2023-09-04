#pragma once

// defer is from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
#define DEFER_CONCAT_INTERNAL(x,y) x##y
#define DEFER_CONCAT(x,y) DEFER_CONCAT_INTERNAL(x,y)

template<typename T>
struct Defer_Exit_Scope {
    T lambda;
    Defer_Exit_Scope(T lambda) : lambda(lambda) {}
    ~Defer_Exit_Scope() {lambda();}
    Defer_Exit_Scope(const Defer_Exit_Scope&);
private:
    Defer_Exit_Scope& operator =(const Defer_Exit_Scope&);
};

struct Defer_Exit_Scope_Help {
    template<typename T>
    Defer_Exit_Scope<T> operator+(T t) {return t;}
};

#define defer const auto& DEFER_CONCAT(defer__, __LINE__) = Defer_Exit_Scope_Help() + [&]()
