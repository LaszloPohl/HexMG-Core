//***********************************************************************
// HexMG Vektor Header
// Creation date:  2021. 05. 25.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HMG_VEKTOR_HEADER
#define	HMG_VEKTOR_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgException.h"
#include <iostream>
#include <iomanip>
#include <vector>
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


#ifdef NDEBUG // in release mode the original vektor is used

//***********************************************************************
template<typename datatype> class vektor {
//***********************************************************************
    datatype* arr;
    unsigned n;
    bool to_be_deleted;
    //***********************************************************************
    vektor(const vektor&) = delete;
    vektor(const vektor&&) = delete;
    void operator=(const vektor&&) = delete;
    //***********************************************************************
public:
    //***********************************************************************
    vektor() noexcept :arr{ nullptr }, n{ 0 }, to_be_deleted{ false } {}
    //***********************************************************************
    ~vektor() { clear_without_setting_member_variables(); }
    //***********************************************************************
    unsigned size()const noexcept { return n; }
    //***********************************************************************
    void clear_without_setting_member_variables() noexcept { if (to_be_deleted)delete[]arr; }
    //***********************************************************************
    void clear() noexcept { clear_without_setting_member_variables(); arr = nullptr; n = 0; to_be_deleted = false; }
    //***********************************************************************
    void set_size(unsigned new_size) { clear_without_setting_member_variables(); n = new_size; arr = (n == 0) ? nullptr : new datatype[n]; to_be_deleted = true; }
    void resize_if_needed(unsigned new_size) { if (n != new_size)set_size(new_size); }
    //***********************************************************************
    void set_size_and_zero(unsigned new_size) { set_size(new_size); zero(); }
    //***********************************************************************
    datatype& operator[](unsigned i) noexcept(!hmgVErrorCheck) { if constexpr(hmgVErrorCheck)is_smaller_error(i, n, "vektor::operator[]"); return arr[i]; }
    //***********************************************************************
    const datatype& operator[](unsigned i)const noexcept(!hmgVErrorCheck) { if constexpr (hmgVErrorCheck) is_smaller_error(i, n, "vektor::operator[]const"); return arr[i]; }
    //***********************************************************************
    const datatype& first()const noexcept(!hmgVErrorCheck) { return operator[](0); }
    //***********************************************************************
    const datatype& last()const noexcept(!hmgVErrorCheck) { return operator[](n-1); }
    //***********************************************************************
    datatype& last() noexcept(!hmgVErrorCheck) { return operator[](n - 1); }
    //***********************************************************************
        
    //***********************************************************************
    vektor & operator=(const vektor& theother){
    //***********************************************************************
        set_size(theother.n);
        for (unsigned i = 0; i < n; i++)
            arr[i] = theother.arr[i];
        return *this;
    }


    //***********************************************************************
    void debug_write(::std::ofstream & fs) const{
    //***********************************************************************
        for (unsigned i = 0; i < n; i++) {
            if (i % 128 == 0) {
                fs << (i == 0 ? "   " : "\n>> ");
            }
            fs << ::std::setw(12) << arr[i] << ' ';
        }
        fs << ::std::endl;
    }


    //***********************************************************************
    void print(unsigned start = 0) const{
    //***********************************************************************
        for (unsigned i = 0; i < start; i++)
            ::std::cout << "- ";
        for (unsigned i = start; i < n; i++)
            ::std::cout << arr[i] << ' ';
    }


    //***********************************************************************
    void print_z(unsigned start = 0) const{
    //***********************************************************************
        for (unsigned i = 0; i < start; i++)
            ::std::cout << "-------- ";
        for (unsigned i = start; i < n; i++) {
            if((abs(arr[i]) < 1e-10))
                ::std::cout << "0        ";
            else
                ::std::cout << (abs(arr[i]) < 1e-10 ? 0 : arr[i]) << ' ';
        }
    }


    //***********************************************************************
    void lay(vektor & theother)noexcept {
    // lays a vector on another one, this vektor points to the other.
    //***********************************************************************
        clear_without_setting_member_variables();
        arr = theother.arr;
        n = theother.n;
        to_be_deleted = false;
    }


    //***********************************************************************
    void lay(vektor & theother, unsigned start, unsigned no)noexcept {
    // lays a vector on another one, this vektor points to the other.
    //***********************************************************************
        clear_without_setting_member_variables();
        n = no;
        to_be_deleted = false;
        arr = (no == 0) ? nullptr : (theother.arr + start);
    }


    //***********************************************************************
    bool refresh_unsafe(unsigned i, const datatype & thenew)noexcept { // return: false, if not refreshed (equals)
    //***********************************************************************
        double diff = abs(arr[i] - thenew);
        if (diff == 0)
            return false;
        arr[i] = thenew;
        return true;
    }


    //***********************************************************************
    bool refresh_unsafe(const vektor& src)noexcept { // return: false, if not refreshed (equals)
    //***********************************************************************
        bool isRefreshed = false;
        for (unsigned i = 0; i < n; i++)
            isRefreshed = refresh_unsafe(i, src.arr[i]) || isRefreshed;
        return isRefreshed;
    }


    //***********************************************************************
    void copy_unsafe(const vektor& src)noexcept {
    //***********************************************************************
        for (unsigned i = 0; i < n; i++)
            arr[i] = src.arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    void subvektor_copy(const vektor & src, unsigned start_dest, unsigned start_src, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return;
        is_smaller_error(start_dest + no - 1, n,     "vektor::subvektor_copy start_dest + no");
        is_smaller_error(start_src  + no - 1, src.n, "vektor::subvektor_copy start_src + no");
        for (unsigned i = 0; i < no; i++)
            arr[start_dest + i] = src.arr[start_src + i];
    }


    //***********************************************************************
    void subvektor_plus_equal(const vektor & src, unsigned start_dest, unsigned start_src, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return;
        is_smaller_error(start_dest + no - 1, n,     "vektor::subvektor_plus_equal start_dest + no");
        is_smaller_error(start_src  + no - 1, src.n, "vektor::subvektor_plus_equal start_src + no");
        for (unsigned i = 0; i < no; i++)
            arr[start_dest + i] += src.arr[start_src + i];
    }


    //***********************************************************************
    bool subvektor_refresh(const vektor & src, unsigned start_dest, unsigned start_src, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return false;
        is_smaller_error(start_dest + no - 1, n,     "vektor::subvektor_refresh start_dest + no");
        is_smaller_error(start_src  + no - 1, src.n, "vektor::subvektor_refresh start_src + no");
        bool is_changed = false;
        for (unsigned i = 0; i < no; i++)
            is_changed = refresh_unsafe(start_dest + i, src.arr[start_src + i]) || is_changed;
        return is_changed;
    }


    //***********************************************************************
    void subvektor_add(const vektor & src_1, const vektor & src_2, unsigned start_dest, unsigned start_src_1, unsigned start_src_2, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return;
        is_smaller_error(start_dest  + no - 1, n,       "vektor::subvektor_copy start_dest + no");
        is_smaller_error(start_src_1 + no - 1, src_1.n, "vektor::subvektor_copy start_src_1 + no");
        is_smaller_error(start_src_2 + no - 1, src_2.n, "vektor::subvektor_copy start_src_2 + no");
        for (unsigned i = 0; i < no; i++)
            arr[start_dest + i] = src_1.arr[start_src_1 + i] + src_2.arr[start_src_2 + i];
    }


    //***********************************************************************
    void push_back(const datatype & thenew){
    //***********************************************************************
        datatype*t2 = new datatype[n + 1];
        for (unsigned i = 0; i < n; i++)
            t2[i] = arr[i];
        if (to_be_deleted)
            delete[]arr;
        arr = t2;
        arr[n] = thenew;
        n++;
        to_be_deleted = true;
    }


    //***********************************************************************
    void zero()noexcept {
    //***********************************************************************
        for (unsigned i = 0; i < n;i++)
            arr[i] = datatype();
    }


    //***********************************************************************
    void math_add(const vektor & v1, const vektor & v2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        is_equal_error(v1.n, v2.n, "vektor::add v1.n!=v2.n");
        is_equal_error(this->n, v2.n, "vektor::add this->n!=input.n ");
        for (unsigned i = 0; i < n; i++)
            arr[i] = v1.arr[i] + v2.arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    void math_sub(const vektor & v1, const vektor & v2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        is_equal_error(v1.n, v2.n, "vektor::sub v1.n!=v2.n");
        is_equal_error(this->n, v2.n, "vektor::sub this->n!=input.n ");
        for (unsigned i = 0; i < n; i++)
            arr[i] = v1.arr[i] - v2.arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    void math_neg() noexcept {
    //***********************************************************************
        for (unsigned i = 0; i < n; i++)
            arr[i] = -arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    friend inline datatype math_mul(const vektor & v1, const vektor & v2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        is_equal_error(v1.n, v2.n, "vektor mul v1.n!=v2.n");
        datatype sum = datatype();
        for (unsigned i = 0; i < v1.n; i++)
            sum += v1.arr[i] * v2.arr[i];
        return sum;
    }
    //***********************************************************************
};

#else

//***********************************************************************
template<typename datatype> class vektor {
//***********************************************************************
    std::vector<datatype> vec;
    datatype* arr = nullptr;
    unsigned n = 0;
    bool to_be_deleted = false;
    //***********************************************************************
    //vektor(const vektor&) = delete;
    //vektor(const vektor&&) = delete;
    //void operator=(const vektor&&) = delete;
    //***********************************************************************
public:
    //***********************************************************************
    vektor() = default;
    //***********************************************************************
    unsigned size()const noexcept { return n; }
    //***********************************************************************
    void clear_without_setting_member_variables() noexcept { if (to_be_deleted)vec.clear(); }
    //***********************************************************************
    void clear() noexcept { clear_without_setting_member_variables(); arr = nullptr; n = 0; to_be_deleted = false; }
    //***********************************************************************
    void set_size(unsigned new_size) { clear_without_setting_member_variables(); n = new_size; vec.resize(n); arr = (n == 0) ? nullptr : &vec[0]; to_be_deleted = true; }
    void resize_if_needed(unsigned new_size) { if (n != new_size)set_size(new_size); }
    //***********************************************************************
    void set_size_and_zero(unsigned new_size) { set_size(new_size); zero(); }
    //***********************************************************************
    datatype& operator[](unsigned i) noexcept(!hmgVErrorCheck) { if constexpr (hmgVErrorCheck)is_smaller_error(i, n, "vektor::operator[]"); return arr[i]; }
    //***********************************************************************
    const datatype& operator[](unsigned i)const noexcept(!hmgVErrorCheck) { if constexpr (hmgVErrorCheck) is_smaller_error(i, n, "vektor::operator[]const"); return arr[i]; }
    //***********************************************************************
    const datatype& first()const noexcept(!hmgVErrorCheck) { return operator[](0); }
    //***********************************************************************
    const datatype& last()const noexcept(!hmgVErrorCheck) { return operator[](n-1); }
    //***********************************************************************
    datatype& last() noexcept(!hmgVErrorCheck) { return operator[](n - 1); }
    //***********************************************************************
        
    //***********************************************************************
    vektor & operator=(const vektor& theother){
    //***********************************************************************
        set_size(theother.n);
        for (unsigned i = 0; i < n; i++)
            arr[i] = theother.arr[i];
        return *this;
    }


    //***********************************************************************
    void debug_write(::std::ofstream & fs) const{
    //***********************************************************************
        for (unsigned i = 0; i < n; i++) {
            if (i % 128 == 0) {
                fs << (i == 0 ? "   " : "\n>> ");
            }
            fs << ::std::setw(12) << arr[i] << ' ';
        }
        fs << ::std::endl;
    }


    //***********************************************************************
    void print(unsigned start = 0) const{
    //***********************************************************************
        for (unsigned i = 0; i < start; i++)
            ::std::cout << "- ";
        for (unsigned i = start; i < n; i++)
            ::std::cout << arr[i] << ' ';
    }


    //***********************************************************************
    void print_z(unsigned start = 0) const{
    //***********************************************************************
        for (unsigned i = 0; i < start; i++)
            ::std::cout << "-------- ";
        for (unsigned i = start; i < n; i++) {
            if((abs(arr[i]) < 1e-10))
                ::std::cout << "0        ";
            else
                ::std::cout << (abs(arr[i]) < 1e-10 ? 0 : arr[i]) << ' ';
        }
    }


    //***********************************************************************
    void lay(vektor & theother)noexcept {
    // lays a vector on another one, this vektor points to the other.
    //***********************************************************************
        clear_without_setting_member_variables();
        arr = theother.arr;
        n = theother.n;
        to_be_deleted = false;
    }


    //***********************************************************************
    void lay(vektor & theother, unsigned start, unsigned no)noexcept {
    // lays a vector on another one, this vektor points to the other.
    //***********************************************************************
        clear_without_setting_member_variables();
        n = no;
        to_be_deleted = false;
        arr = (no == 0) ? nullptr : (theother.arr + start);
    }


    //***********************************************************************
    bool refresh_unsafe(unsigned i, const datatype & thenew)noexcept { // return: false, if not refreshed (equals)
    //***********************************************************************
        double diff = abs(arr[i] - thenew);
        if (diff == 0)
            return false;
        arr[i] = thenew;
        return true;
    }


    //***********************************************************************
    bool refresh_unsafe(const vektor& src)noexcept { // return: false, if not refreshed (equals)
    //***********************************************************************
        bool isRefreshed = false;
        for (unsigned i = 0; i < n; i++)
            isRefreshed = refresh_unsafe(i, src.arr[i]) || isRefreshed;
        return isRefreshed;
    }


    //***********************************************************************
    void copy_unsafe(const vektor& src)noexcept {
    //***********************************************************************
        for (unsigned i = 0; i < n; i++)
            arr[i] = src.arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    void subvektor_copy(const vektor & src, unsigned start_dest, unsigned start_src, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return;
        is_smaller_error(start_dest + no - 1, n,     "vektor::subvektor_copy start_dest + no");
        is_smaller_error(start_src  + no - 1, src.n, "vektor::subvektor_copy start_src + no");
        for (unsigned i = 0; i < no; i++)
            arr[start_dest + i] = src.arr[start_src + i];
    }


    //***********************************************************************
    void subvektor_plus_equal(const vektor & src, unsigned start_dest, unsigned start_src, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return;
        is_smaller_error(start_dest + no - 1, n,     "vektor::subvektor_plus_equal start_dest + no");
        is_smaller_error(start_src  + no - 1, src.n, "vektor::subvektor_plus_equal start_src + no");
        for (unsigned i = 0; i < no; i++)
            arr[start_dest + i] += src.arr[start_src + i];
    }


    //***********************************************************************
    bool subvektor_refresh(const vektor & src, unsigned start_dest, unsigned start_src, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return false;
        is_smaller_error(start_dest + no - 1, n,     "vektor::subvektor_refresh start_dest + no");
        is_smaller_error(start_src  + no - 1, src.n, "vektor::subvektor_refresh start_src + no");
        bool is_changed = false;
        for (unsigned i = 0; i < no; i++)
            is_changed = refresh_unsafe(start_dest + i, src.arr[start_src + i]) || is_changed;
        return is_changed;
    }


    //***********************************************************************
    void subvektor_add(const vektor & src_1, const vektor & src_2, unsigned start_dest, unsigned start_src_1, unsigned start_src_2, unsigned no) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no == 0)
            return;
        is_smaller_error(start_dest  + no - 1, n,       "vektor::subvektor_copy start_dest + no");
        is_smaller_error(start_src_1 + no - 1, src_1.n, "vektor::subvektor_copy start_src_1 + no");
        is_smaller_error(start_src_2 + no - 1, src_2.n, "vektor::subvektor_copy start_src_2 + no");
        for (unsigned i = 0; i < no; i++)
            arr[start_dest + i] = src_1.arr[start_src_1 + i] + src_2.arr[start_src_2 + i];
    }


    //***********************************************************************
    void push_back(const datatype & thenew){
    //***********************************************************************
        if (arr == &vec[0]) {
            vec.resize(n + 1);
        }
        else {
            datatype* t2 = new datatype[n];
            for (unsigned i = 0; i < n; i++)
                t2[i] = arr[i];
            vec.resize(n + 1);
            for (unsigned i = 0; i < n; i++)
                vec[i] = arr[i];
            delete[]t2;
        }
        arr = &vec[0];
        arr[n] = thenew;
        n++;
        to_be_deleted = true;
    }


    //***********************************************************************
    void zero()noexcept {
    //***********************************************************************
        for (unsigned i = 0; i < n;i++)
            arr[i] = datatype();
    }


    //***********************************************************************
    void math_add(const vektor & v1, const vektor & v2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        is_equal_error(v1.n, v2.n, "vektor::add v1.n!=v2.n");
        is_equal_error(this->n, v2.n, "vektor::add this->n!=input.n ");
        for (unsigned i = 0; i < n; i++)
            arr[i] = v1.arr[i] + v2.arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    void math_sub(const vektor & v1, const vektor & v2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        is_equal_error(v1.n, v2.n, "vektor::sub v1.n!=v2.n");
        is_equal_error(this->n, v2.n, "vektor::sub this->n!=input.n ");
        for (unsigned i = 0; i < n; i++)
            arr[i] = v1.arr[i] - v2.arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    void math_neg() noexcept {
    //***********************************************************************
        for (unsigned i = 0; i < n; i++)
            arr[i] = -arr[i];
    }
    //***********************************************************************


    //***********************************************************************
    friend inline datatype math_mul(const vektor & v1, const vektor & v2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        is_equal_error(v1.n, v2.n, "vektor mul v1.n!=v2.n");
        datatype sum = datatype();
        for (unsigned i = 0; i < v1.n; i++)
            sum += v1.arr[i] * v2.arr[i];
        return sum;
    }
    //***********************************************************************
};


#endif

}

#endif

