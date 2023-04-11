//***********************************************************************
// HexMG exception header
// Creation date:  2023. 01. 24.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_EXCEPTION_HEADER
#define	HMG_EXCEPTION_HEADER
//***********************************************************************
#include <cstdio>
#include <cstdarg>
#include <exception>
#include <string>
#include <fstream>
#include <sstream>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
#ifdef NDEBUG
    inline constexpr bool hmgVErrorCheck = false;
#else
    inline constexpr bool hmgVErrorCheck = true;
#endif
//***********************************************************************


//***********************************************************************
class hmgExcept : public std::exception {
//***********************************************************************
    char t[1024];
public:

    //***********************************************************************
    hmgExcept(const char* who_threw, const char* error_message, ...) {
    //***********************************************************************
        va_list p;
        char s[1024];

        va_start(p, error_message);
        sprintf_s(s, 1024, "Runtime error: %s => %s\n", who_threw, error_message);
        vsprintf_s(t, 1024, s, p);
        va_end(p);
    }
    //***********************************************************************

    //***********************************************************************
    const char* what()const noexcept override { return t; }
    //***********************************************************************

};
//***********************************************************************


//***********************************************************************
#define todo_error
//***********************************************************************


//***********************************************************************
template<typename T>
inline void is_equal_error(T num_1, T num_2, const char * who_threw) noexcept(!hmgVErrorCheck) {
//***********************************************************************
#ifndef NDEBUG
    if (num_1 != num_2) {
        std::stringstream ss;
        ss << num_1 << "!=" << num_2;
        throw hmgExcept(who_threw, ss.str().c_str());
    }
#endif
}


//***********************************************************************
template<typename T>
inline void is_smaller_error(T num_1, T num_2, const char * who_threw) noexcept(!hmgVErrorCheck) {
//***********************************************************************
#ifndef NDEBUG
    if (num_1 >= num_2) {
        std::stringstream ss;
        ss << num_1 << ">=" << num_2;
        throw hmgExcept(who_threw, ss.str().c_str());
    }
#endif
}


//***********************************************************************
template<typename T>
inline void is_bigger_error(T num_1, T num_2, const char * who_threw) noexcept(!hmgVErrorCheck) {
//***********************************************************************
#ifndef NDEBUG
    if (num_1 <= num_2) {
        std::stringstream ss;
        ss << num_1 << "<=" << num_2;
        throw hmgExcept(who_threw, ss.str().c_str());
    }
#endif
}


//***********************************************************************
inline void is_true_error(bool is_hiba, const char * who_threw, const char * what) noexcept(!hmgVErrorCheck) {
//***********************************************************************
#ifndef NDEBUG
    if (is_hiba) {
        throw hmgExcept(who_threw, what);
    }
#endif
}


//***********************************************************************
inline void TODO(const char * what) {
//***********************************************************************
#ifdef todo_error
    throw hmgExcept("TODO", what);
#endif
}

}


//***********************************************************************
#endif
//***********************************************************************
