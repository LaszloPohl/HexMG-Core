#include "hmgCommon.h"
#include "hmgMatrix.hpp"
#include <stdexcept>
using namespace nsHMG;
static unsigned checks=0;
template<class T> double magnitude(const T& x) {return static_cast<double>(abs(x));}
template<> double magnitude(const dd_real& x) {return to_double(abs(x));}
template<class T> void run(const T& factor) {
    for(unsigned n:{1u,2u,3u,4u,5u,8u}) for(int method=0;method<3;++method) {
        matrix<T> a,original;a.set_size(n,n);original.set_size(n,n);
        for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j) {
            // Strictly dominant, nontrivial SPD matrix; an equal scale must
            // cancel in A*A^-1 even far below the old absolute pivot floor.
            a[i][j]=original[i][j]=factor*T(i==j?4.0:1.0/(1.0+std::abs(int(i)-int(j))));
        }
        if(method==0)a.math_ninv_np_();
        if(method==1)a.math_ninv_np();
        if(method==2)a.math_symm_ninv_of_nonsymm();
        for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){T value(0.0);
            for(unsigned k=0;k<n;++k)value+=original[i][k]*a[k][j];
            ++checks;if(magnitude(value+T(i==j?1:0))>3e-12)
                throw std::runtime_error("Scale-dependent inverse: n="+std::to_string(n)+" method="+std::to_string(method));
        }
    }
}
int main(){try{
    for(double scale:{1.0,1e-12,1e-23,1e-100,1e100}){
        run<double>(scale);run<double>(-scale);run<dd_real>(dd_real(scale));
        run<std::complex<double>>({scale,scale*.3});
    }
    std::cout<<checks<<" scale-invariant matrix checks PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
