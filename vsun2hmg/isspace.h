//***********************************************************************
// isspace header
// Creation date:  2009. 07. 12.
// Creator:        Pohl L�szl�
//***********************************************************************


//***********************************************************************
#ifndef PL_ISSPACE_HEADER
#define	PL_ISSPACE_HEADER
//***********************************************************************


//***********************************************************************
inline bool plisspace(int ch){
//***********************************************************************
    if(ch>=0x09&&ch<=0x0D||ch==0x20)return true;
    return false;
}

#endif