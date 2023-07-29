//***********************************************************************
// HexMG Matrix Header
// Creation date:  2021. 05. 25.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#ifndef HMG_MATRIX_HEADER
#define	HMG_MATRIX_HEADER
//***********************************************************************


//***********************************************************************
#include "hmgVektor.hpp"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
template<typename datatype> class matrix;
//***********************************************************************


//***********************************************************************
template<typename datatype> class matrix {
//***********************************************************************
    
    //***********************************************************************
    vektor<datatype> t;
    vektor< vektor<datatype> > rows;
    unsigned row, col;
    bool is_symm;
    //***********************************************************************

    //***********************************************************************
    void set_rows() {
    //***********************************************************************
        rows.set_size(row);
        if (is_symm) {
            unsigned start = 0, no = row;
            for (unsigned i = 0; i < row; i++) {
                rows[i].lay(t, start - i, col); // so m[i][j] is indeed the element with index i,j, but j>=i is mandatory! 
                start += no;
                no--;
            }
        }
        else {
            for (unsigned i = 0; i < row; i++)
                rows[i].lay(t, i*col, col);
        }
    }

    //***********************************************************************
    matrix(const matrix&) = delete;
    matrix(const matrix&&) = delete;
    void operator=(const matrix&&) = delete;
    //***********************************************************************
public:
    
    //***********************************************************************
    const matrix & operator=(const matrix & theother) {
    // Not lay-proof.
    //***********************************************************************
        t = theother.t;
        row = theother.row;
        col = theother.col;
        is_symm = theother.is_symm;
        set_rows();
        return *this;
    }
    //***********************************************************************
    matrix() noexcept :row{ 0 }, col{ 0 }, is_symm{ false } {}
    //***********************************************************************
    ~matrix() {}
    //***********************************************************************
    unsigned get_col() const noexcept { return col; }
    //***********************************************************************
    unsigned get_row() const noexcept { return row; }
    //***********************************************************************
    bool get_is_symm() const noexcept { return is_symm; }
    //***********************************************************************
    unsigned size() const noexcept { return t.size(); }
    //***********************************************************************
    void clear() noexcept {
    //***********************************************************************
        t.clear();
        rows.clear();
        row = col = 0;
        is_symm = false;
    }
    //***********************************************************************
    bool refresh_unsafe(unsigned i, const datatype & thenew) noexcept { return t.refresh_unsafe(i, thenew); }
    //***********************************************************************
    bool refresh_unsafe(unsigned row, unsigned col, const datatype & thenew) noexcept { return rows[row].refresh_unsafe(col, thenew); }
    //***********************************************************************
    bool refresh_unsafe(const matrix& src) {
    //***********************************************************************
        if (col == 0 || row == 0)
            return false;
        if (is_symm != src.is_symm || row != src.row || col != src.col)
            throw hmgExcept("matrix::refresh_unsafe(matrix)", "src and dest matrix sizes are different");
        return t.refresh_unsafe(src.t);
    }
    //***********************************************************************
    void set_size(unsigned new_row, unsigned new_col) { clear(); row = new_row; col = new_col; is_symm = false; t.set_size(row*col); set_rows(); }
    //***********************************************************************
    void set_size_symm(unsigned new_rowcol) { clear(); row = new_rowcol; col = new_rowcol; is_symm = true; t.set_size((new_rowcol *(new_rowcol + 1)) / 2); set_rows(); }
    //***********************************************************************
    void resize_if_needed(unsigned new_row, unsigned new_col, bool new_symm) {
    //***********************************************************************
        if (new_row != row || new_col != col || new_symm != is_symm) {
            if (new_symm)
                set_size_symm(new_row);
            else
                set_size(new_row, new_col);
        }
    }
    //***********************************************************************
    void set_size_and_zero(unsigned new_row, unsigned new_col) { set_size(new_row, new_col); zero_unsafe(); }
    //***********************************************************************
    void set_size_symm_and_zero(unsigned uj_rowcol) { set_size_symm(uj_rowcol); zero_unsafe(); }
    //***********************************************************************
    void lay(matrix & theother, unsigned start_row, unsigned start_col, unsigned row_no, unsigned col_no, bool is_symm) {
    // t will be empty, so the member functions that use t will not work
    // use rows to address cells
    //***********************************************************************
        is_smaller_error(start_row + row_no - 1, theother.row, "matrix::lay start_row + row_no");
        is_smaller_error(start_col + col_no - 1, theother.col, "matrix::lay start_col + col_no");
        clear();
        rows.set_size(row_no);
        row = row_no;
        col = col_no;
        is_symm = is_symm;
        for (unsigned i = 0; i < row_no; i++)
            rows[i].lay(theother.rows.unsafe(start_row + i), start_col, col_no); // (vektor & theother, unsigned start, unsigned no)
    }
    //***********************************************************************
    // Not lay-proof.
    void math_add_unsafe(const matrix & a, const matrix & b) noexcept(!hmgVErrorCheck) { t.math_add(a.t, b.t); }
    //***********************************************************************
    // Not lay-proof.
    void math_sub_unsafe(const matrix & a, const matrix & b) noexcept(!hmgVErrorCheck) { t.math_sub(a.t, b.t); }
    //***********************************************************************
    // Not lay-proof.
    void math_neg_unsafe() noexcept { t.math_neg(); }
    //***********************************************************************
    vektor<datatype> & operator[](unsigned i) noexcept(!hmgVErrorCheck) { return rows[i]; }
    //***********************************************************************
    const vektor<datatype> & operator[](unsigned i) const noexcept(!hmgVErrorCheck) { return rows[i]; }
    //***********************************************************************
    const datatype & get_elem(unsigned row, unsigned col) const noexcept {
    //***********************************************************************
        if (is_symm) {// so m[i][j] is indeed the element with index i,j, but j>=i is mandatory!
            return (col < row) ? rows[col][row] : rows[row][col];
        }
        else {
            return rows[row][col];
        }
    }
    //***********************************************************************
    datatype & get_elem(unsigned rw, unsigned cl) noexcept {
    //***********************************************************************
        if (is_symm) {// so m[i][j] is indeed the element with index i,j, but j>=i is mandatory!
            return (cl < rw) ? rows[cl][rw] : rows[rw][cl];
        }
        else {
            return rows[rw][cl];
        }
    }


    //***********************************************************************
    void debug_write(::std::ofstream & fs) const{
    //***********************************************************************
        for (unsigned i = 0; i < row; i++){
            rows[i].debug_write(fs);
        }            
    }
    
    //***********************************************************************
    void print() const{
    //***********************************************************************
        for (unsigned i = 0; i < row; i++){
            rows[i].print(is_symm ? i : 0);
            ::std::cout << ::std::endl;
        }            
        ::std::cout << ::std::endl;
    }
    
    //***********************************************************************
    void print_z() const{
    //***********************************************************************
        for (unsigned i = 0; i < row; i++){
            rows[i].print_z(is_symm ? i : 0);
            ::std::cout << ::std::endl;
        }            
        ::std::cout << ::std::endl;
    }
    
    //***********************************************************************
    void print_size(char be = ' ', char end = '\n') const {
    //***********************************************************************
        printf("%c(%u,%u:%u)%c", be, row, col, t.size(), end);
        //::std::cout << be << '(' << row << ',' << col << ':' << t.size() << ')' << end;
    }
    
    //***********************************************************************
    void math_mul_t_unsafe(const matrix & a, const matrix & b_t) noexcept(!hmgVErrorCheck) {
    // Not lay-proof.
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.row, row,     "math_mul_t row row");
        is_equal_error(b_t.row, col,   "math_mul_t row col");
        is_equal_error(a.col, b_t.col, "math_mul_t col col");
        is_true_error(is_symm || a.is_symm || b_t.is_symm, "matrix::math_mul_t", "symmetrical matrix not allowed");

        if (row == 0 || col == 0)
            return;
        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        datatype *d0 = &t[0], *d1 = d0 + nj, *d2 = d0 + 2 * nj, *d3 = d0 + 3 * nj;
        const datatype *a0 = &a.t[0], *a1 = a0 + nk, *a2 = a0 + 2 * nk, *a3 = a0 + 3 * nk;
        for (unsigned i = 0; i < hi; i += 4) {
            const datatype *b0 = &b_t.t[0], *b1 = b0 + nk, *b2 = b0 + 2 * nk, *b3 = b0 + 3 * nk;
            for (unsigned j = 0; j < hj; j += 4) {
                datatype d[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4, a0 += 4, a1 += 4, a2 += 4, a3 += 4, b0 += 4, b1 += 4, b2 += 4, b3 += 4) {
                    d[0]  += a0[0] * b0[0] + a0[1] * b0[1] + a0[2] * b0[2] + a0[3] * b0[3];
                    d[1]  += a0[0] * b1[0] + a0[1] * b1[1] + a0[2] * b1[2] + a0[3] * b1[3];
                    d[2]  += a0[0] * b2[0] + a0[1] * b2[1] + a0[2] * b2[2] + a0[3] * b2[3];
                    d[3]  += a0[0] * b3[0] + a0[1] * b3[1] + a0[2] * b3[2] + a0[3] * b3[3];

                    d[4]  += a1[0] * b0[0] + a1[1] * b0[1] + a1[2] * b0[2] + a1[3] * b0[3];
                    d[5]  += a1[0] * b1[0] + a1[1] * b1[1] + a1[2] * b1[2] + a1[3] * b1[3];
                    d[6]  += a1[0] * b2[0] + a1[1] * b2[1] + a1[2] * b2[2] + a1[3] * b2[3];
                    d[7]  += a1[0] * b3[0] + a1[1] * b3[1] + a1[2] * b3[2] + a1[3] * b3[3];

                    d[8]  += a2[0] * b0[0] + a2[1] * b0[1] + a2[2] * b0[2] + a2[3] * b0[3];
                    d[9]  += a2[0] * b1[0] + a2[1] * b1[1] + a2[2] * b1[2] + a2[3] * b1[3];
                    d[10] += a2[0] * b2[0] + a2[1] * b2[1] + a2[2] * b2[2] + a2[3] * b2[3];
                    d[11] += a2[0] * b3[0] + a2[1] * b3[1] + a2[2] * b3[2] + a2[3] * b3[3];

                    d[12] += a3[0] * b0[0] + a3[1] * b0[1] + a3[2] * b0[2] + a3[3] * b0[3];
                    d[13] += a3[0] * b1[0] + a3[1] * b1[1] + a3[2] * b1[2] + a3[3] * b1[3];
                    d[14] += a3[0] * b2[0] + a3[1] * b2[1] + a3[2] * b2[2] + a3[3] * b2[3];
                    d[15] += a3[0] * b3[0] + a3[1] * b3[1] + a3[2] * b3[2] + a3[3] * b3[3];
                }
                for (unsigned k = 0; k < dk; k++) {
                    d[0]  += a0[k] * b0[k];	d[1]  += a0[k] * b1[k];	d[2]  += a0[k] * b2[k];	d[3]  += a0[k] * b3[k];
                    d[4]  += a1[k] * b0[k];	d[5]  += a1[k] * b1[k];	d[6]  += a1[k] * b2[k];	d[7]  += a1[k] * b3[k];
                    d[8]  += a2[k] * b0[k];	d[9]  += a2[k] * b1[k];	d[10] += a2[k] * b2[k];	d[11] += a2[k] * b3[k];
                    d[12] += a3[k] * b0[k];	d[13] += a3[k] * b1[k];	d[14] += a3[k] * b2[k];	d[15] += a3[k] * b3[k];
                }
                d0[0] = d[0];  d0[1] = d[1];  d0[2] = d[2];  d0[3] = d[3];
                d1[0] = d[4];  d1[1] = d[5];  d1[2] = d[6];  d1[3] = d[7];
                d2[0] = d[8];  d2[1] = d[9];  d2[2] = d[10]; d2[3] = d[11];
                d3[0] = d[12]; d3[1] = d[13]; d3[2] = d[14]; d3[3] = d[15];
                d0 += 4, d1 += 4, d2 += 4, d3 += 4;
                a0 -= hk, a1 -= hk, a2 -= hk, a3 -= hk, b0 += dk + 3 * nk, b1 += dk + 3 * nk, b2 += dk + 3 * nk, b3 += dk + 3 * nk;
            }
            for (unsigned j = 0; j < dj; j++) {
                d0[j] = d1[j] = d2[j] = d3[j] = 0;
                for (unsigned k = 0; k < nk; k++) {
                    d0[j] += a0[k] * b0[j*nk + k]; d1[j] += a1[k] * b0[j*nk + k]; d2[j] += a2[k] * b0[j*nk + k]; d3[j] += a3[k] * b0[j*nk + k];
                }
            }
            d0 += dj + 3 * nj; d1 += dj + 3 * nj; d2 += dj + 3 * nj; d3 += dj + 3 * nj;
            a0 += 4 * nk; a1 += 4 * nk; a2 += 4 * nk; a3 += 4 * nk;
        }
        const datatype *b0 = &b_t.t[0];
        for (unsigned i = 0; i < di; i++)
            for (unsigned j = 0; j < nj; j++) {
                d0[i*nj + j] = 0;
                for (unsigned k = 0; k < nk; k++)
                    d0[i*nj + j] += a0[i*nk + k] * b0[j*nk + k];
            }
    }

    //***********************************************************************
    void math_mul_t_safe(const matrix & a, const matrix & b_t) noexcept(!hmgVErrorCheck) {
    // about 4% slower than the pointer version, but it works on a layed matrix
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.row, row,     "math_mul_t row row");
        is_equal_error(b_t.row, col,   "math_mul_t row col");
        is_equal_error(a.col, b_t.col, "math_mul_t col col");
        is_true_error(is_symm || a.is_symm || b_t.is_symm, "matrix::math_mul_t", "symmetrical matrix not allowed");

        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        for (unsigned i = 0; i < hi; i += 4) {
            vektor<datatype> & dest_row_i0 = rows[i + 0];
            vektor<datatype> & dest_row_i1 = rows[i + 1];
            vektor<datatype> & dest_row_i2 = rows[i + 2];
            vektor<datatype> & dest_row_i3 = rows[i + 3];
            const vektor<datatype> & a_row_i0 = a.rows[i + 0];
            const vektor<datatype> & a_row_i1 = a.rows[i + 1];
            const vektor<datatype> & a_row_i2 = a.rows[i + 2];
            const vektor<datatype> & a_row_i3 = a.rows[i + 3];
            for (unsigned j = 0; j < hj; j += 4) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j + 0];
                const vektor<datatype> & b_row_j1 = b_t.rows[j + 1];
                const vektor<datatype> & b_row_j2 = b_t.rows[j + 2];
                const vektor<datatype> & b_row_j3 = b_t.rows[j + 3];
                datatype d[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4) {
                    d[0]  += a_row_i0[k + 0] * b_row_j0[k + 0] 
                           + a_row_i0[k + 1] * b_row_j0[k + 1] 
                           + a_row_i0[k + 2] * b_row_j0[k + 2] 
                           + a_row_i0[k + 3] * b_row_j0[k + 3];
                    d[1]  += a_row_i0[k + 0] * b_row_j1[k + 0] 
                           + a_row_i0[k + 1] * b_row_j1[k + 1] 
                           + a_row_i0[k + 2] * b_row_j1[k + 2] 
                           + a_row_i0[k + 3] * b_row_j1[k + 3];
                    d[2]  += a_row_i0[k + 0] * b_row_j2[k + 0] 
                           + a_row_i0[k + 1] * b_row_j2[k + 1] 
                           + a_row_i0[k + 2] * b_row_j2[k + 2] 
                           + a_row_i0[k + 3] * b_row_j2[k + 3];
                    d[3]  += a_row_i0[k + 0] * b_row_j3[k + 0] 
                           + a_row_i0[k + 1] * b_row_j3[k + 1] 
                           + a_row_i0[k + 2] * b_row_j3[k + 2] 
                           + a_row_i0[k + 3] * b_row_j3[k + 3];

                    d[4]  += a_row_i1[k + 0] * b_row_j0[k + 0] 
                           + a_row_i1[k + 1] * b_row_j0[k + 1]
                           + a_row_i1[k + 2] * b_row_j0[k + 2]
                           + a_row_i1[k + 3] * b_row_j0[k + 3];
                    d[5]  += a_row_i1[k + 0] * b_row_j1[k + 0]
                           + a_row_i1[k + 1] * b_row_j1[k + 1]
                           + a_row_i1[k + 2] * b_row_j1[k + 2]
                           + a_row_i1[k + 3] * b_row_j1[k + 3];
                    d[6]  += a_row_i1[k + 0] * b_row_j2[k + 0]
                           + a_row_i1[k + 1] * b_row_j2[k + 1]
                           + a_row_i1[k + 2] * b_row_j2[k + 2]
                           + a_row_i1[k + 3] * b_row_j2[k + 3];
                    d[7]  += a_row_i1[k + 0] * b_row_j3[k + 0]
                           + a_row_i1[k + 1] * b_row_j3[k + 1]
                           + a_row_i1[k + 2] * b_row_j3[k + 2]
                           + a_row_i1[k + 3] * b_row_j3[k + 3];

                    d[8]  += a_row_i2[k + 0] * b_row_j0[k + 0] 
                           + a_row_i2[k + 1] * b_row_j0[k + 1]
                           + a_row_i2[k + 2] * b_row_j0[k + 2]
                           + a_row_i2[k + 3] * b_row_j0[k + 3];
                    d[9]  += a_row_i2[k + 0] * b_row_j1[k + 0]
                           + a_row_i2[k + 1] * b_row_j1[k + 1]
                           + a_row_i2[k + 2] * b_row_j1[k + 2]
                           + a_row_i2[k + 3] * b_row_j1[k + 3];
                    d[10] += a_row_i2[k + 0] * b_row_j2[k + 0]
                           + a_row_i2[k + 1] * b_row_j2[k + 1]
                           + a_row_i2[k + 2] * b_row_j2[k + 2]
                           + a_row_i2[k + 3] * b_row_j2[k + 3];
                    d[11] += a_row_i2[k + 0] * b_row_j3[k + 0]
                           + a_row_i2[k + 1] * b_row_j3[k + 1]
                           + a_row_i2[k + 2] * b_row_j3[k + 2]
                           + a_row_i2[k + 3] * b_row_j3[k + 3];

                    d[12] += a_row_i3[k + 0] * b_row_j0[k + 0] 
                           + a_row_i3[k + 1] * b_row_j0[k + 1]
                           + a_row_i3[k + 2] * b_row_j0[k + 2]
                           + a_row_i3[k + 3] * b_row_j0[k + 3];
                    d[13] += a_row_i3[k + 0] * b_row_j1[k + 0]
                           + a_row_i3[k + 1] * b_row_j1[k + 1]
                           + a_row_i3[k + 2] * b_row_j1[k + 2]
                           + a_row_i3[k + 3] * b_row_j1[k + 3];
                    d[14] += a_row_i3[k + 0] * b_row_j2[k + 0]
                           + a_row_i3[k + 1] * b_row_j2[k + 1]
                           + a_row_i3[k + 2] * b_row_j2[k + 2]
                           + a_row_i3[k + 3] * b_row_j2[k + 3];
                    d[15] += a_row_i3[k + 0] * b_row_j3[k + 0]
                           + a_row_i3[k + 1] * b_row_j3[k + 1]
                           + a_row_i3[k + 2] * b_row_j3[k + 2]
                           + a_row_i3[k + 3] * b_row_j3[k + 3];
                }
                for (unsigned k = hk; k < nk; k++) {
                    d[0]  += a_row_i0[k] * b_row_j0[k];
                    d[1]  += a_row_i0[k] * b_row_j1[k];
                    d[2]  += a_row_i0[k] * b_row_j2[k];
                    d[3]  += a_row_i0[k] * b_row_j3[k];

                    d[4]  += a_row_i1[k] * b_row_j0[k];
                    d[5]  += a_row_i1[k] * b_row_j1[k];
                    d[6]  += a_row_i1[k] * b_row_j2[k];
                    d[7]  += a_row_i1[k] * b_row_j3[k];

                    d[8]  += a_row_i2[k] * b_row_j0[k];
                    d[9]  += a_row_i2[k] * b_row_j1[k];
                    d[10] += a_row_i2[k] * b_row_j2[k];
                    d[11] += a_row_i2[k] * b_row_j3[k];

                    d[12] += a_row_i3[k] * b_row_j0[k];
                    d[13] += a_row_i3[k] * b_row_j1[k];
                    d[14] += a_row_i3[k] * b_row_j2[k];
                    d[15] += a_row_i3[k] * b_row_j3[k];
                }
                dest_row_i0[j + 0] = d[0];
                dest_row_i0[j + 1] = d[1];
                dest_row_i0[j + 2] = d[2];
                dest_row_i0[j + 3] = d[3];

                dest_row_i1[j + 0] = d[4];
                dest_row_i1[j + 1] = d[5];
                dest_row_i1[j + 2] = d[6];
                dest_row_i1[j + 3] = d[7];

                dest_row_i2[j + 0] = d[8];
                dest_row_i2[j + 1] = d[9];
                dest_row_i2[j + 2] = d[10];
                dest_row_i2[j + 3] = d[11];

                dest_row_i3[j + 0] = d[12];
                dest_row_i3[j + 1] = d[13];
                dest_row_i3[j + 2] = d[14];
                dest_row_i3[j + 3] = d[15];
            }
            for (unsigned j = hj; j < nj; j++) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j];
                datatype d[4] = { datatype() };
                for (unsigned k = 0; k < nk; k++) {
                    d[0] += a_row_i0[k] * b_row_j0[k];
                    d[1] += a_row_i1[k] * b_row_j0[k];
                    d[2] += a_row_i2[k] * b_row_j0[k];
                    d[3] += a_row_i3[k] * b_row_j0[k];
                }
                dest_row_i0[j] = d[0];
                dest_row_i1[j] = d[1];
                dest_row_i2[j] = d[2];
                dest_row_i3[j] = d[3];
            }
        }
        for (unsigned i = hi; i < ni; i++) {
            vektor<datatype> & dest_row_i = rows[i];
            const vektor<datatype> & a_row_i = a.rows[i];
            for (unsigned j = 0; j < nj; j++) {
                const vektor<datatype> & b_row_j = b_t.rows[j];
                datatype d = datatype();
                for (unsigned k = 0; k < nk; k++)
                    d += a_row_i[k] * b_row_j[k];
                dest_row_i[j] = d;
            }
        }
    }

    //***********************************************************************
    void math_nmul_t_safe(const matrix & a, const matrix & b_t) noexcept(!hmgVErrorCheck) {
    // also runs on a layed matrix, giving -1 times the multiplication
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.row, row,     "math_nmul_t row row");
        is_equal_error(b_t.row, col,   "math_nmul_t row col");
        is_equal_error(a.col, b_t.col, "math_nmul_t col col");
        is_true_error(is_symm || a.is_symm || b_t.is_symm, "matrix::math_nmul_t", "symmetrical matrix not allowed");

        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        for (unsigned i = 0; i < hi; i += 4) {
            vektor<datatype> & dest_row_i0 = rows[i + 0];
            vektor<datatype> & dest_row_i1 = rows[i + 1];
            vektor<datatype> & dest_row_i2 = rows[i + 2];
            vektor<datatype> & dest_row_i3 = rows[i + 3];
            const vektor<datatype> & a_row_i0 = a.rows[i + 0];
            const vektor<datatype> & a_row_i1 = a.rows[i + 1];
            const vektor<datatype> & a_row_i2 = a.rows[i + 2];
            const vektor<datatype> & a_row_i3 = a.rows[i + 3];
            for (unsigned j = 0; j < hj; j += 4) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j + 0];
                const vektor<datatype> & b_row_j1 = b_t.rows[j + 1];
                const vektor<datatype> & b_row_j2 = b_t.rows[j + 2];
                const vektor<datatype> & b_row_j3 = b_t.rows[j + 3];
                datatype d[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4) {
                    d[0]  += a_row_i0[k + 0] * b_row_j0[k + 0] 
                           + a_row_i0[k + 1] * b_row_j0[k + 1] 
                           + a_row_i0[k + 2] * b_row_j0[k + 2] 
                           + a_row_i0[k + 3] * b_row_j0[k + 3];
                    d[1]  += a_row_i0[k + 0] * b_row_j1[k + 0] 
                           + a_row_i0[k + 1] * b_row_j1[k + 1] 
                           + a_row_i0[k + 2] * b_row_j1[k + 2] 
                           + a_row_i0[k + 3] * b_row_j1[k + 3];
                    d[2]  += a_row_i0[k + 0] * b_row_j2[k + 0] 
                           + a_row_i0[k + 1] * b_row_j2[k + 1] 
                           + a_row_i0[k + 2] * b_row_j2[k + 2] 
                           + a_row_i0[k + 3] * b_row_j2[k + 3];
                    d[3]  += a_row_i0[k + 0] * b_row_j3[k + 0] 
                           + a_row_i0[k + 1] * b_row_j3[k + 1] 
                           + a_row_i0[k + 2] * b_row_j3[k + 2] 
                           + a_row_i0[k + 3] * b_row_j3[k + 3];

                    d[4]  += a_row_i1[k + 0] * b_row_j0[k + 0] 
                           + a_row_i1[k + 1] * b_row_j0[k + 1]
                           + a_row_i1[k + 2] * b_row_j0[k + 2]
                           + a_row_i1[k + 3] * b_row_j0[k + 3];
                    d[5]  += a_row_i1[k + 0] * b_row_j1[k + 0]
                           + a_row_i1[k + 1] * b_row_j1[k + 1]
                           + a_row_i1[k + 2] * b_row_j1[k + 2]
                           + a_row_i1[k + 3] * b_row_j1[k + 3];
                    d[6]  += a_row_i1[k + 0] * b_row_j2[k + 0]
                           + a_row_i1[k + 1] * b_row_j2[k + 1]
                           + a_row_i1[k + 2] * b_row_j2[k + 2]
                           + a_row_i1[k + 3] * b_row_j2[k + 3];
                    d[7]  += a_row_i1[k + 0] * b_row_j3[k + 0]
                           + a_row_i1[k + 1] * b_row_j3[k + 1]
                           + a_row_i1[k + 2] * b_row_j3[k + 2]
                           + a_row_i1[k + 3] * b_row_j3[k + 3];

                    d[8]  += a_row_i2[k + 0] * b_row_j0[k + 0] 
                           + a_row_i2[k + 1] * b_row_j0[k + 1]
                           + a_row_i2[k + 2] * b_row_j0[k + 2]
                           + a_row_i2[k + 3] * b_row_j0[k + 3];
                    d[9]  += a_row_i2[k + 0] * b_row_j1[k + 0]
                           + a_row_i2[k + 1] * b_row_j1[k + 1]
                           + a_row_i2[k + 2] * b_row_j1[k + 2]
                           + a_row_i2[k + 3] * b_row_j1[k + 3];
                    d[10] += a_row_i2[k + 0] * b_row_j2[k + 0]
                           + a_row_i2[k + 1] * b_row_j2[k + 1]
                           + a_row_i2[k + 2] * b_row_j2[k + 2]
                           + a_row_i2[k + 3] * b_row_j2[k + 3];
                    d[11] += a_row_i2[k + 0] * b_row_j3[k + 0]
                           + a_row_i2[k + 1] * b_row_j3[k + 1]
                           + a_row_i2[k + 2] * b_row_j3[k + 2]
                           + a_row_i2[k + 3] * b_row_j3[k + 3];

                    d[12] += a_row_i3[k + 0] * b_row_j0[k + 0] 
                           + a_row_i3[k + 1] * b_row_j0[k + 1]
                           + a_row_i3[k + 2] * b_row_j0[k + 2]
                           + a_row_i3[k + 3] * b_row_j0[k + 3];
                    d[13] += a_row_i3[k + 0] * b_row_j1[k + 0]
                           + a_row_i3[k + 1] * b_row_j1[k + 1]
                           + a_row_i3[k + 2] * b_row_j1[k + 2]
                           + a_row_i3[k + 3] * b_row_j1[k + 3];
                    d[14] += a_row_i3[k + 0] * b_row_j2[k + 0]
                           + a_row_i3[k + 1] * b_row_j2[k + 1]
                           + a_row_i3[k + 2] * b_row_j2[k + 2]
                           + a_row_i3[k + 3] * b_row_j2[k + 3];
                    d[15] += a_row_i3[k + 0] * b_row_j3[k + 0]
                           + a_row_i3[k + 1] * b_row_j3[k + 1]
                           + a_row_i3[k + 2] * b_row_j3[k + 2]
                           + a_row_i3[k + 3] * b_row_j3[k + 3];
                }
                for (unsigned k = hk; k < nk; k++) {
                    d[0]  += a_row_i0[k] * b_row_j0[k];
                    d[1]  += a_row_i0[k] * b_row_j1[k];
                    d[2]  += a_row_i0[k] * b_row_j2[k];
                    d[3]  += a_row_i0[k] * b_row_j3[k];

                    d[4]  += a_row_i1[k] * b_row_j0[k];
                    d[5]  += a_row_i1[k] * b_row_j1[k];
                    d[6]  += a_row_i1[k] * b_row_j2[k];
                    d[7]  += a_row_i1[k] * b_row_j3[k];

                    d[8]  += a_row_i2[k] * b_row_j0[k];
                    d[9]  += a_row_i2[k] * b_row_j1[k];
                    d[10] += a_row_i2[k] * b_row_j2[k];
                    d[11] += a_row_i2[k] * b_row_j3[k];

                    d[12] += a_row_i3[k] * b_row_j0[k];
                    d[13] += a_row_i3[k] * b_row_j1[k];
                    d[14] += a_row_i3[k] * b_row_j2[k];
                    d[15] += a_row_i3[k] * b_row_j3[k];
                }
                dest_row_i0[j + 0] = -d[0];
                dest_row_i0[j + 1] = -d[1];
                dest_row_i0[j + 2] = -d[2];
                dest_row_i0[j + 3] = -d[3];

                dest_row_i1[j + 0] = -d[4];
                dest_row_i1[j + 1] = -d[5];
                dest_row_i1[j + 2] = -d[6];
                dest_row_i1[j + 3] = -d[7];

                dest_row_i2[j + 0] = -d[8];
                dest_row_i2[j + 1] = -d[9];
                dest_row_i2[j + 2] = -d[10];
                dest_row_i2[j + 3] = -d[11];

                dest_row_i3[j + 0] = -d[12];
                dest_row_i3[j + 1] = -d[13];
                dest_row_i3[j + 2] = -d[14];
                dest_row_i3[j + 3] = -d[15];
            }
            for (unsigned j = hj; j < nj; j++) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j];
                datatype d[4] = { datatype() };
                for (unsigned k = 0; k < nk; k++) {
                    d[0] += a_row_i0[k] * b_row_j0[k];
                    d[1] += a_row_i1[k] * b_row_j0[k];
                    d[2] += a_row_i2[k] * b_row_j0[k];
                    d[3] += a_row_i3[k] * b_row_j0[k];
                }
                dest_row_i0[j] = -d[0];
                dest_row_i1[j] = -d[1];
                dest_row_i2[j] = -d[2];
                dest_row_i3[j] = -d[3];
            }
        }
        for (unsigned i = hi; i < ni; i++) {
            vektor<datatype> & dest_row_i = rows[i];
            const vektor<datatype> & a_row_i = a.rows[i];
            for (unsigned j = 0; j < nj; j++) {
                const vektor<datatype> & b_row_j = b_t.rows[j];
                datatype d = datatype();
                for (unsigned k = 0; k < nk; k++)
                    d += a_row_i[k] * b_row_j[k];
                dest_row_i[j] = -d;
            }
        }
    }

    //***********************************************************************
    void math_add_mul_t_unsafe(const matrix & c, const matrix & a, const matrix & b_t) noexcept(!hmgVErrorCheck) {
    // Not lay-proof.
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(c.col, col,     "math_add_mul_t col");
        is_equal_error(c.row, row,     "math_add_mul_t row");
        is_equal_error(a.row, row,     "math_add_mul_t row row");
        is_equal_error(b_t.row, col,   "math_add_mul_t row col");
        is_equal_error(a.col, b_t.col, "math_add_mul_t col col");
        is_true_error(is_symm || a.is_symm || b_t.is_symm, "matrix::math_add_mul_t", "symmetrical matrix not allowed");

        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        datatype *d0 = &t[0], *d1 = d0 + nj, *d2 = d0 + 2 * nj, *d3 = d0 + 3 * nj;
        const datatype *c0 = &c.t[0], *c1 = c0 + nj, *c2 = c0 + 2 * nj, *c3 = c0 + 3 * nj;
        const datatype *a0 = &a.t[0], *a1 = a0 + nk, *a2 = a0 + 2 * nk, *a3 = a0 + 3 * nk;
        for (unsigned i = 0; i < hi; i += 4) {
            const datatype *b0 = &b_t.t[0], *b1 = b0 + nk, *b2 = b0 + 2 * nk, *b3 = b0 + 3 * nk;
            for (unsigned j = 0; j < hj; j += 4) {
                datatype d[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4, a0 += 4, a1 += 4, a2 += 4, a3 += 4, b0 += 4, b1 += 4, b2 += 4, b3 += 4) {
                    d[0] += a0[0] * b0[0] + a0[1] * b0[1] + a0[2] * b0[2] + a0[3] * b0[3];
                    d[1] += a0[0] * b1[0] + a0[1] * b1[1] + a0[2] * b1[2] + a0[3] * b1[3];
                    d[2] += a0[0] * b2[0] + a0[1] * b2[1] + a0[2] * b2[2] + a0[3] * b2[3];
                    d[3] += a0[0] * b3[0] + a0[1] * b3[1] + a0[2] * b3[2] + a0[3] * b3[3];

                    d[4] += a1[0] * b0[0] + a1[1] * b0[1] + a1[2] * b0[2] + a1[3] * b0[3];
                    d[5] += a1[0] * b1[0] + a1[1] * b1[1] + a1[2] * b1[2] + a1[3] * b1[3];
                    d[6] += a1[0] * b2[0] + a1[1] * b2[1] + a1[2] * b2[2] + a1[3] * b2[3];
                    d[7] += a1[0] * b3[0] + a1[1] * b3[1] + a1[2] * b3[2] + a1[3] * b3[3];

                    d[8] += a2[0] * b0[0] + a2[1] * b0[1] + a2[2] * b0[2] + a2[3] * b0[3];
                    d[9] += a2[0] * b1[0] + a2[1] * b1[1] + a2[2] * b1[2] + a2[3] * b1[3];
                    d[10] += a2[0] * b2[0] + a2[1] * b2[1] + a2[2] * b2[2] + a2[3] * b2[3];
                    d[11] += a2[0] * b3[0] + a2[1] * b3[1] + a2[2] * b3[2] + a2[3] * b3[3];

                    d[12] += a3[0] * b0[0] + a3[1] * b0[1] + a3[2] * b0[2] + a3[3] * b0[3];
                    d[13] += a3[0] * b1[0] + a3[1] * b1[1] + a3[2] * b1[2] + a3[3] * b1[3];
                    d[14] += a3[0] * b2[0] + a3[1] * b2[1] + a3[2] * b2[2] + a3[3] * b2[3];
                    d[15] += a3[0] * b3[0] + a3[1] * b3[1] + a3[2] * b3[2] + a3[3] * b3[3];
                }
                for (unsigned k = 0; k < dk; k++) {
                    d[0] += a0[k] * b0[k];	d[1] += a0[k] * b1[k];	d[2] += a0[k] * b2[k];	d[3] += a0[k] * b3[k];
                    d[4] += a1[k] * b0[k];	d[5] += a1[k] * b1[k];	d[6] += a1[k] * b2[k];	d[7] += a1[k] * b3[k];
                    d[8] += a2[k] * b0[k];	d[9] += a2[k] * b1[k];	d[10] += a2[k] * b2[k];	d[11] += a2[k] * b3[k];
                    d[12] += a3[k] * b0[k];	d[13] += a3[k] * b1[k];	d[14] += a3[k] * b2[k];	d[15] += a3[k] * b3[k];
                }
                d0[0] = d[0]  + c0[0]; d0[1] = d[1]  + c0[1]; d0[2] = d[2]  + c0[2]; d0[3] = d[3]  + c0[3];
                d1[0] = d[4]  + c1[0]; d1[1] = d[5]  + c1[1]; d1[2] = d[6]  + c1[2]; d1[3] = d[7]  + c1[3];
                d2[0] = d[8]  + c2[0]; d2[1] = d[9]  + c2[1]; d2[2] = d[10] + c2[2]; d2[3] = d[11] + c2[3];
                d3[0] = d[12] + c3[0]; d3[1] = d[13] + c3[1]; d3[2] = d[14] + c3[2]; d3[3] = d[15] + c3[3];
                d0 += 4, d1 += 4, d2 += 4, d3 += 4;
                c0 += 4, c1 += 4, c2 += 4, c3 += 4;
                a0 -= hk, a1 -= hk, a2 -= hk, a3 -= hk, b0 += dk + 3 * nk, b1 += dk + 3 * nk, b2 += dk + 3 * nk, b3 += dk + 3 * nk;
            }
            for (unsigned j = 0; j < dj; j++) {
                d0[j] = c0[j]; d1[j] = c1[j]; d2[j] = c2[j]; d3[j] = c3[j];
                for (unsigned k = 0; k < nk; k++) {
                    d0[j] += a0[k] * b0[j*nk + k]; d1[j] += a1[k] * b0[j*nk + k]; d2[j] += a2[k] * b0[j*nk + k]; d3[j] += a3[k] * b0[j*nk + k];
                }
            }
            d0 += dj + 3 * nj; d1 += dj + 3 * nj; d2 += dj + 3 * nj; d3 += dj + 3 * nj;
            c0 += dj + 3 * nj; c1 += dj + 3 * nj; c2 += dj + 3 * nj; c3 += dj + 3 * nj;
            a0 += 4 * nk; a1 += 4 * nk; a2 += 4 * nk; a3 += 4 * nk;
        }
        const datatype *b0 = &b_t.t[0];
        for (unsigned i = 0; i < di; i++)
            for (unsigned j = 0; j < nj; j++) {
                d0[i*nj + j] = c0[i*nj + j];
                for (unsigned k = 0; k < nk; k++)
                    d0[i*nj + j] += a0[i*nk + k] * b0[j*nk + k];
            }
    }

    //***********************************************************************
    void math_add_mul_t_safe(const matrix & c, const matrix & a, const matrix & b_t) noexcept(!hmgVErrorCheck) {
    // about 4% slower than the pointer version, but it works on a layed matrix
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.row, row,     "math_add_mul_t_ row row");
        is_equal_error(b_t.row, col,   "math_add_mul_t_ row col");
        is_equal_error(a.col, b_t.col, "math_add_mul_t_ col col");
        is_true_error(is_symm || a.is_symm || b_t.is_symm, "matrix::math_add_mul_t_", "symmetrical matrix not allowed");

        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        for (unsigned i = 0; i < hi; i += 4) {
            vektor<datatype> & dest_row_i0 = rows[i + 0];
            vektor<datatype> & dest_row_i1 = rows[i + 1];
            vektor<datatype> & dest_row_i2 = rows[i + 2];
            vektor<datatype> & dest_row_i3 = rows[i + 3];
            const vektor<datatype> & a_row_i0 = a.rows[i + 0];
            const vektor<datatype> & a_row_i1 = a.rows[i + 1];
            const vektor<datatype> & a_row_i2 = a.rows[i + 2];
            const vektor<datatype> & a_row_i3 = a.rows[i + 3];
            const vektor<datatype> & c_sor_i0 = c.rows[i + 0];
            const vektor<datatype> & c_sor_i1 = c.rows[i + 1];
            const vektor<datatype> & c_sor_i2 = c.rows[i + 2];
            const vektor<datatype> & c_sor_i3 = c.rows[i + 3];
            for (unsigned j = 0; j < hj; j += 4) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j + 0];
                const vektor<datatype> & b_row_j1 = b_t.rows[j + 1];
                const vektor<datatype> & b_row_j2 = b_t.rows[j + 2];
                const vektor<datatype> & b_row_j3 = b_t.rows[j + 3];
                datatype d[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4) {
                    d[0]  += a_row_i0[k + 0] * b_row_j0[k + 0] 
                           + a_row_i0[k + 1] * b_row_j0[k + 1] 
                           + a_row_i0[k + 2] * b_row_j0[k + 2] 
                           + a_row_i0[k + 3] * b_row_j0[k + 3];
                    d[1]  += a_row_i0[k + 0] * b_row_j1[k + 0] 
                           + a_row_i0[k + 1] * b_row_j1[k + 1] 
                           + a_row_i0[k + 2] * b_row_j1[k + 2] 
                           + a_row_i0[k + 3] * b_row_j1[k + 3];
                    d[2]  += a_row_i0[k + 0] * b_row_j2[k + 0] 
                           + a_row_i0[k + 1] * b_row_j2[k + 1] 
                           + a_row_i0[k + 2] * b_row_j2[k + 2] 
                           + a_row_i0[k + 3] * b_row_j2[k + 3];
                    d[3]  += a_row_i0[k + 0] * b_row_j3[k + 0] 
                           + a_row_i0[k + 1] * b_row_j3[k + 1] 
                           + a_row_i0[k + 2] * b_row_j3[k + 2] 
                           + a_row_i0[k + 3] * b_row_j3[k + 3];

                    d[4]  += a_row_i1[k + 0] * b_row_j0[k + 0] 
                           + a_row_i1[k + 1] * b_row_j0[k + 1]
                           + a_row_i1[k + 2] * b_row_j0[k + 2]
                           + a_row_i1[k + 3] * b_row_j0[k + 3];
                    d[5]  += a_row_i1[k + 0] * b_row_j1[k + 0]
                           + a_row_i1[k + 1] * b_row_j1[k + 1]
                           + a_row_i1[k + 2] * b_row_j1[k + 2]
                           + a_row_i1[k + 3] * b_row_j1[k + 3];
                    d[6]  += a_row_i1[k + 0] * b_row_j2[k + 0]
                           + a_row_i1[k + 1] * b_row_j2[k + 1]
                           + a_row_i1[k + 2] * b_row_j2[k + 2]
                           + a_row_i1[k + 3] * b_row_j2[k + 3];
                    d[7]  += a_row_i1[k + 0] * b_row_j3[k + 0]
                           + a_row_i1[k + 1] * b_row_j3[k + 1]
                           + a_row_i1[k + 2] * b_row_j3[k + 2]
                           + a_row_i1[k + 3] * b_row_j3[k + 3];

                    d[8]  += a_row_i2[k + 0] * b_row_j0[k + 0] 
                           + a_row_i2[k + 1] * b_row_j0[k + 1]
                           + a_row_i2[k + 2] * b_row_j0[k + 2]
                           + a_row_i2[k + 3] * b_row_j0[k + 3];
                    d[9]  += a_row_i2[k + 0] * b_row_j1[k + 0]
                           + a_row_i2[k + 1] * b_row_j1[k + 1]
                           + a_row_i2[k + 2] * b_row_j1[k + 2]
                           + a_row_i2[k + 3] * b_row_j1[k + 3];
                    d[10] += a_row_i2[k + 0] * b_row_j2[k + 0]
                           + a_row_i2[k + 1] * b_row_j2[k + 1]
                           + a_row_i2[k + 2] * b_row_j2[k + 2]
                           + a_row_i2[k + 3] * b_row_j2[k + 3];
                    d[11] += a_row_i2[k + 0] * b_row_j3[k + 0]
                           + a_row_i2[k + 1] * b_row_j3[k + 1]
                           + a_row_i2[k + 2] * b_row_j3[k + 2]
                           + a_row_i2[k + 3] * b_row_j3[k + 3];

                    d[12] += a_row_i3[k + 0] * b_row_j0[k + 0] 
                           + a_row_i3[k + 1] * b_row_j0[k + 1]
                           + a_row_i3[k + 2] * b_row_j0[k + 2]
                           + a_row_i3[k + 3] * b_row_j0[k + 3];
                    d[13] += a_row_i3[k + 0] * b_row_j1[k + 0]
                           + a_row_i3[k + 1] * b_row_j1[k + 1]
                           + a_row_i3[k + 2] * b_row_j1[k + 2]
                           + a_row_i3[k + 3] * b_row_j1[k + 3];
                    d[14] += a_row_i3[k + 0] * b_row_j2[k + 0]
                           + a_row_i3[k + 1] * b_row_j2[k + 1]
                           + a_row_i3[k + 2] * b_row_j2[k + 2]
                           + a_row_i3[k + 3] * b_row_j2[k + 3];
                    d[15] += a_row_i3[k + 0] * b_row_j3[k + 0]
                           + a_row_i3[k + 1] * b_row_j3[k + 1]
                           + a_row_i3[k + 2] * b_row_j3[k + 2]
                           + a_row_i3[k + 3] * b_row_j3[k + 3];
                }
                for (unsigned k = hk; k < nk; k++) {
                    d[0]  += a_row_i0[k] * b_row_j0[k];
                    d[1]  += a_row_i0[k] * b_row_j1[k];
                    d[2]  += a_row_i0[k] * b_row_j2[k];
                    d[3]  += a_row_i0[k] * b_row_j3[k];

                    d[4]  += a_row_i1[k] * b_row_j0[k];
                    d[5]  += a_row_i1[k] * b_row_j1[k];
                    d[6]  += a_row_i1[k] * b_row_j2[k];
                    d[7]  += a_row_i1[k] * b_row_j3[k];

                    d[8]  += a_row_i2[k] * b_row_j0[k];
                    d[9]  += a_row_i2[k] * b_row_j1[k];
                    d[10] += a_row_i2[k] * b_row_j2[k];
                    d[11] += a_row_i2[k] * b_row_j3[k];

                    d[12] += a_row_i3[k] * b_row_j0[k];
                    d[13] += a_row_i3[k] * b_row_j1[k];
                    d[14] += a_row_i3[k] * b_row_j2[k];
                    d[15] += a_row_i3[k] * b_row_j3[k];
                }
                dest_row_i0[j + 0] = d[0] + c_sor_i0[j + 0];
                dest_row_i0[j + 1] = d[1] + c_sor_i0[j + 1];
                dest_row_i0[j + 2] = d[2] + c_sor_i0[j + 2];
                dest_row_i0[j + 3] = d[3] + c_sor_i0[j + 3];

                dest_row_i1[j + 0] = d[4] + c_sor_i1[j + 0];
                dest_row_i1[j + 1] = d[5] + c_sor_i1[j + 1];
                dest_row_i1[j + 2] = d[6] + c_sor_i1[j + 2];
                dest_row_i1[j + 3] = d[7] + c_sor_i1[j + 3];

                dest_row_i2[j + 0] = d[8] + c_sor_i2[j + 0];
                dest_row_i2[j + 1] = d[9] + c_sor_i2[j + 1];
                dest_row_i2[j + 2] = d[10] + c_sor_i2[j + 2];
                dest_row_i2[j + 3] = d[11] + c_sor_i2[j + 3];

                dest_row_i3[j + 0] = d[12] + c_sor_i3[j + 0];
                dest_row_i3[j + 1] = d[13] + c_sor_i3[j + 1];
                dest_row_i3[j + 2] = d[14] + c_sor_i3[j + 2];
                dest_row_i3[j + 3] = d[15] + c_sor_i3[j + 3];
            }
            for (unsigned j = hj; j < nj; j++) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j];
                datatype d[4] = { datatype() };
                for (unsigned k = 0; k < nk; k++) {
                    d[0] += a_row_i0[k] * b_row_j0[k];
                    d[1] += a_row_i1[k] * b_row_j0[k];
                    d[2] += a_row_i2[k] * b_row_j0[k];
                    d[3] += a_row_i3[k] * b_row_j0[k];
                }
                dest_row_i0[j] = d[0] + c_sor_i0[j];
                dest_row_i1[j] = d[1] + c_sor_i1[j];
                dest_row_i2[j] = d[2] + c_sor_i2[j];
                dest_row_i3[j] = d[3] + c_sor_i3[j];
            }
        }
        for (unsigned i = hi; i < ni; i++) {
            vektor<datatype> & dest_row_i = rows[i];
            const vektor<datatype> & a_row_i = a.rows[i];
            const vektor<datatype> & c_sor_i = c.rows[i];
            for (unsigned j = 0; j < nj; j++) {
                const vektor<datatype> & b_row_j = b_t.rows[j];
                datatype d = datatype();
                for (unsigned k = 0; k < nk; k++)
                    d += a_row_i[k] * b_row_j[k];
                dest_row_i[j] = d + c_sor_i[j];
            }
        }
    }

    //***********************************************************************
    void math_sub_mul_t_safe(const matrix & c, const matrix & a, const matrix & b_t) noexcept(!hmgVErrorCheck) {
    // also works on a layed matrix, *this = c - a * b_t 
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.row, row,     "math_sub_mul_t row row");
        is_equal_error(b_t.row, col,   "math_sub_mul_t row col");
        is_equal_error(a.col, b_t.col, "math_sub_mul_t col col");
        is_true_error(is_symm || a.is_symm || b_t.is_symm, "matrix::math_sub_mul_t", "symmetrical matrix not allowed");

        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        for (unsigned i = 0; i < hi; i += 4) {
            vektor<datatype> & dest_row_i0 = rows[i + 0];
            vektor<datatype> & dest_row_i1 = rows[i + 1];
            vektor<datatype> & dest_row_i2 = rows[i + 2];
            vektor<datatype> & dest_row_i3 = rows[i + 3];
            const vektor<datatype> & a_row_i0 = a.rows[i + 0];
            const vektor<datatype> & a_row_i1 = a.rows[i + 1];
            const vektor<datatype> & a_row_i2 = a.rows[i + 2];
            const vektor<datatype> & a_row_i3 = a.rows[i + 3];
            const vektor<datatype> & c_sor_i0 = c.rows[i + 0];
            const vektor<datatype> & c_sor_i1 = c.rows[i + 1];
            const vektor<datatype> & c_sor_i2 = c.rows[i + 2];
            const vektor<datatype> & c_sor_i3 = c.rows[i + 3];
            for (unsigned j = 0; j < hj; j += 4) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j + 0];
                const vektor<datatype> & b_row_j1 = b_t.rows[j + 1];
                const vektor<datatype> & b_row_j2 = b_t.rows[j + 2];
                const vektor<datatype> & b_row_j3 = b_t.rows[j + 3];
                datatype d[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4) {
                    d[0]  += a_row_i0[k + 0] * b_row_j0[k + 0] 
                           + a_row_i0[k + 1] * b_row_j0[k + 1] 
                           + a_row_i0[k + 2] * b_row_j0[k + 2] 
                           + a_row_i0[k + 3] * b_row_j0[k + 3];
                    d[1]  += a_row_i0[k + 0] * b_row_j1[k + 0] 
                           + a_row_i0[k + 1] * b_row_j1[k + 1] 
                           + a_row_i0[k + 2] * b_row_j1[k + 2] 
                           + a_row_i0[k + 3] * b_row_j1[k + 3];
                    d[2]  += a_row_i0[k + 0] * b_row_j2[k + 0] 
                           + a_row_i0[k + 1] * b_row_j2[k + 1] 
                           + a_row_i0[k + 2] * b_row_j2[k + 2] 
                           + a_row_i0[k + 3] * b_row_j2[k + 3];
                    d[3]  += a_row_i0[k + 0] * b_row_j3[k + 0] 
                           + a_row_i0[k + 1] * b_row_j3[k + 1] 
                           + a_row_i0[k + 2] * b_row_j3[k + 2] 
                           + a_row_i0[k + 3] * b_row_j3[k + 3];

                    d[4]  += a_row_i1[k + 0] * b_row_j0[k + 0] 
                           + a_row_i1[k + 1] * b_row_j0[k + 1]
                           + a_row_i1[k + 2] * b_row_j0[k + 2]
                           + a_row_i1[k + 3] * b_row_j0[k + 3];
                    d[5]  += a_row_i1[k + 0] * b_row_j1[k + 0]
                           + a_row_i1[k + 1] * b_row_j1[k + 1]
                           + a_row_i1[k + 2] * b_row_j1[k + 2]
                           + a_row_i1[k + 3] * b_row_j1[k + 3];
                    d[6]  += a_row_i1[k + 0] * b_row_j2[k + 0]
                           + a_row_i1[k + 1] * b_row_j2[k + 1]
                           + a_row_i1[k + 2] * b_row_j2[k + 2]
                           + a_row_i1[k + 3] * b_row_j2[k + 3];
                    d[7]  += a_row_i1[k + 0] * b_row_j3[k + 0]
                           + a_row_i1[k + 1] * b_row_j3[k + 1]
                           + a_row_i1[k + 2] * b_row_j3[k + 2]
                           + a_row_i1[k + 3] * b_row_j3[k + 3];

                    d[8]  += a_row_i2[k + 0] * b_row_j0[k + 0] 
                           + a_row_i2[k + 1] * b_row_j0[k + 1]
                           + a_row_i2[k + 2] * b_row_j0[k + 2]
                           + a_row_i2[k + 3] * b_row_j0[k + 3];
                    d[9]  += a_row_i2[k + 0] * b_row_j1[k + 0]
                           + a_row_i2[k + 1] * b_row_j1[k + 1]
                           + a_row_i2[k + 2] * b_row_j1[k + 2]
                           + a_row_i2[k + 3] * b_row_j1[k + 3];
                    d[10] += a_row_i2[k + 0] * b_row_j2[k + 0]
                           + a_row_i2[k + 1] * b_row_j2[k + 1]
                           + a_row_i2[k + 2] * b_row_j2[k + 2]
                           + a_row_i2[k + 3] * b_row_j2[k + 3];
                    d[11] += a_row_i2[k + 0] * b_row_j3[k + 0]
                           + a_row_i2[k + 1] * b_row_j3[k + 1]
                           + a_row_i2[k + 2] * b_row_j3[k + 2]
                           + a_row_i2[k + 3] * b_row_j3[k + 3];

                    d[12] += a_row_i3[k + 0] * b_row_j0[k + 0] 
                           + a_row_i3[k + 1] * b_row_j0[k + 1]
                           + a_row_i3[k + 2] * b_row_j0[k + 2]
                           + a_row_i3[k + 3] * b_row_j0[k + 3];
                    d[13] += a_row_i3[k + 0] * b_row_j1[k + 0]
                           + a_row_i3[k + 1] * b_row_j1[k + 1]
                           + a_row_i3[k + 2] * b_row_j1[k + 2]
                           + a_row_i3[k + 3] * b_row_j1[k + 3];
                    d[14] += a_row_i3[k + 0] * b_row_j2[k + 0]
                           + a_row_i3[k + 1] * b_row_j2[k + 1]
                           + a_row_i3[k + 2] * b_row_j2[k + 2]
                           + a_row_i3[k + 3] * b_row_j2[k + 3];
                    d[15] += a_row_i3[k + 0] * b_row_j3[k + 0]
                           + a_row_i3[k + 1] * b_row_j3[k + 1]
                           + a_row_i3[k + 2] * b_row_j3[k + 2]
                           + a_row_i3[k + 3] * b_row_j3[k + 3];
                }
                for (unsigned k = hk; k < nk; k++) {
                    d[0]  += a_row_i0[k] * b_row_j0[k];
                    d[1]  += a_row_i0[k] * b_row_j1[k];
                    d[2]  += a_row_i0[k] * b_row_j2[k];
                    d[3]  += a_row_i0[k] * b_row_j3[k];

                    d[4]  += a_row_i1[k] * b_row_j0[k];
                    d[5]  += a_row_i1[k] * b_row_j1[k];
                    d[6]  += a_row_i1[k] * b_row_j2[k];
                    d[7]  += a_row_i1[k] * b_row_j3[k];

                    d[8]  += a_row_i2[k] * b_row_j0[k];
                    d[9]  += a_row_i2[k] * b_row_j1[k];
                    d[10] += a_row_i2[k] * b_row_j2[k];
                    d[11] += a_row_i2[k] * b_row_j3[k];

                    d[12] += a_row_i3[k] * b_row_j0[k];
                    d[13] += a_row_i3[k] * b_row_j1[k];
                    d[14] += a_row_i3[k] * b_row_j2[k];
                    d[15] += a_row_i3[k] * b_row_j3[k];
                }
                dest_row_i0[j + 0] = c_sor_i0[j + 0] - d[0];
                dest_row_i0[j + 1] = c_sor_i0[j + 1] - d[1];
                dest_row_i0[j + 2] = c_sor_i0[j + 2] - d[2];
                dest_row_i0[j + 3] = c_sor_i0[j + 3] - d[3];

                dest_row_i1[j + 0] = c_sor_i1[j + 0] - d[4];
                dest_row_i1[j + 1] = c_sor_i1[j + 1] - d[5];
                dest_row_i1[j + 2] = c_sor_i1[j + 2] - d[6];
                dest_row_i1[j + 3] = c_sor_i1[j + 3] - d[7];

                dest_row_i2[j + 0] = c_sor_i2[j + 0] - d[8];
                dest_row_i2[j + 1] = c_sor_i2[j + 1] - d[9];
                dest_row_i2[j + 2] = c_sor_i2[j + 2] - d[10];
                dest_row_i2[j + 3] = c_sor_i2[j + 3] - d[11];

                dest_row_i3[j + 0] = c_sor_i3[j + 0] - d[12];
                dest_row_i3[j + 1] = c_sor_i3[j + 1] - d[13];
                dest_row_i3[j + 2] = c_sor_i3[j + 2] - d[14];
                dest_row_i3[j + 3] = c_sor_i3[j + 3] - d[15];
            }
            for (unsigned j = hj; j < nj; j++) {
                const vektor<datatype> & b_row_j0 = b_t.rows[j];
                datatype d[4] = { datatype() };
                for (unsigned k = 0; k < nk; k++) {
                    d[0] += a_row_i0[k] * b_row_j0[k];
                    d[1] += a_row_i1[k] * b_row_j0[k];
                    d[2] += a_row_i2[k] * b_row_j0[k];
                    d[3] += a_row_i3[k] * b_row_j0[k];
                }
                dest_row_i0[j] = c_sor_i0[j] - d[0];
                dest_row_i1[j] = c_sor_i1[j] - d[1];
                dest_row_i2[j] = c_sor_i2[j] - d[2];
                dest_row_i3[j] = c_sor_i3[j] - d[3];
            }
        }
        for (unsigned i = hi; i < ni; i++) {
            vektor<datatype> & dest_row_i = rows[i];
            const vektor<datatype> & a_row_i = a.rows[i];
            const vektor<datatype> & c_sor_i = c.rows[i];
            for (unsigned j = 0; j < nj; j++) {
                const vektor<datatype> & b_row_j = b_t.rows[j];
                datatype d = datatype();
                for (unsigned k = 0; k < nk; k++)
                    d += a_row_i[k] * b_row_j[k];
                dest_row_i[j] = c_sor_i[j] - d;
            }
        }
    }

    //***********************************************************************
    void transp(const matrix & a) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.col, row, "matrix::transp col-row");
        is_equal_error(a.row, col, "matrix::transp row-col");
        is_true_error(is_symm, "matrix::transp", "symmetrical matrix not allowed");
        for (unsigned i = 0; i < row; i++)
            for (unsigned j = 0; j < col; j++)
                rows[i][j] = a.rows[j][i];
    }

    
    //***********************************************************************
    void copy(const matrix & a) noexcept(!hmgVErrorCheck) {
    // also works on a layed matrix
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(a.col, col, "matrix::copy col-col");
        is_equal_error(a.row, row, "matrix::copy row-row");
        is_true_error(is_symm, "matrix::copy", "symmetrical matrix not allowed");
        for (unsigned i = 0; i < row; i++)
            for (unsigned j = 0; j < col; j++)
                rows[i][j] = a.rows[i][j];
    }

    
    //***********************************************************************
    void copy_unsafe(const matrix& src) noexcept {
    //***********************************************************************
        t.copy_unsafe(src.t);
    }

    
     //***********************************************************************
    void copy_from_symm_to_nonsymm(const matrix& src) noexcept(!hmgVErrorCheck) {
    // also works on a layed matrix
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(src.col, col, "matrix::copy_from_symm_to_nonsymm col-col");
        is_equal_error(src.row, row, "matrix::copy_from_symm_to_nonsymm row-row");
        is_true_error(is_symm || !src.is_symm, "matrix::copy_from_symm_to_nonsymm", "symmetrical src and non-symmetrical dest required");
        for (unsigned i = 0; i < row; i++) {
            rows[i][i] = src.rows[i][i];
            for (unsigned j = i + 1; j < col; j++) {
                rows[j][i] = rows[i][j] = src.rows[i][j];
            }
        }
    }

    
   //***********************************************************************
    void submatrix_copy(const matrix & src, unsigned start_dest_row, unsigned start_dest_col, unsigned start_src_row, unsigned start_src_col, unsigned no_row, unsigned no_col) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no_col == 0 || no_row == 0)
            return;
        is_smaller_error(start_dest_row + no_row - 1, row,     "matrix::submatrix_copy start_dest_row + no_row");
        is_smaller_error(start_src_row  + no_row - 1, src.row, "matrix::submatrix_copy start_src_row  + no_row");
        if (is_symm) {
            if (start_dest_row >= start_dest_col + no_col) // i.e. start_dest_row > start_dest_col + no_col - 1
                ; // if the target falls entirely in the lower triangle, discarded
            else {
                if (src.is_symm) {  // symm => symm
                    if (start_dest_row + no_row - 1 <= start_dest_col) { // the target is completely above the main diagonal
                        if (start_src_row + no_row - 1 <= start_src_col) { // the source is completely above the main diagonal
                            for (unsigned i = 0; i < no_row; i++)
                                rows.unsafe(start_dest_row + i).subvektor_copy(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
                        }
                        else if (start_src_row >= start_src_col + no_col) { // the source is completely under the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row, dest_row = start_dest_row;
                            for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                                unsigned src_col = start_src_col, dest_col = start_dest_col;
                                for (unsigned j = 0; j < no_col; j++, src_col++, dest_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                            }
                        }
                        else { // the source is intersected by the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row, dest_row = start_dest_row;
                            for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                                unsigned dest_col = start_dest_col;
                                const unsigned ig = start_src_col + no_col;
                                for (unsigned src_col = start_src_col; src_col < src_row && src_col < ig; src_col++, dest_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                                for (unsigned src_col = start_src_col > src_row ? start_src_col : src_row; src_col < ig; src_col++, dest_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                                }
                            }
                        }
                    }
                    else { // the destination is intersected by the main diagonal
                        if (start_src_row + no_row - 1 <= start_src_col) { // the source is completely above the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row;
                            for (unsigned dest_row = start_dest_row; dest_row < start_dest_row + no_row; dest_row++, src_row++) {
                                unsigned dest_col = dest_row > start_dest_col ? dest_row : start_dest_col;
                                unsigned src_col = start_src_col + dest_col - start_dest_col;
                                for (; dest_col < start_dest_col + no_col; dest_col++, src_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                                }
                            }
                        }
                        else if (start_src_row >= start_src_col + no_col) { // the source is completely under the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row;
                            for (unsigned dest_row = start_dest_row; dest_row < start_dest_row + no_row; dest_row++, src_row++) {
                                unsigned dest_col = dest_row > start_dest_col ? dest_row : start_dest_col;
                                unsigned src_col = start_src_col + dest_col - start_dest_col;
                                for (; dest_col < start_dest_col + no_col; dest_col++, src_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                            }
                        }
                        else { // the source is intersected by the main diagonal
                            unsigned src_row = start_src_row;
                            for (unsigned dest_row = start_dest_row; dest_row < start_dest_row + no_row; dest_row++, src_row++) {
                                unsigned dest_col = dest_row > start_dest_col ? dest_row : start_dest_col;
                                unsigned src_col = start_src_col + dest_col - start_dest_col;
                                for (; dest_col < start_dest_col + no_col; dest_col++, src_col++) {
                                    if (src_col >= src_row)
                                        rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                                    else
                                        rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                            }
                        }
                    }
                }
                else { // nonsymm => symm (is it possible?)
                    if (start_dest_row + no_row - 1 <= start_dest_col) { // the destination is completely above the main diagonal
                        for (unsigned i = 0; i < no_row; i++)
                            rows.unsafe(start_dest_row + i).subvektor_copy(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
                    }
                    else { // there is also a section below the main diagonal in the destination
                        unsigned src_row = start_src_row, dest_row = start_dest_row;
                        for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                            unsigned dest_col = start_dest_col >= dest_row ? start_dest_col : dest_row;
                            unsigned src_col = start_src_col + dest_col - start_dest_col;
                            for (; dest_col < start_dest_col + no_col; src_col++, dest_col++) {
                                rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                            }
                        }
                    }
                }
            }
        }
        else {
            if (src.is_symm) { // symm => nonsyimm
                if (start_src_row + no_row - 1 <= start_src_col) { // the source is completely above the main diagonal
                    for (unsigned i = 0; i < no_row; i++)
                        rows.unsafe(start_dest_row + i).subvektor_copy(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
                }
                else if (start_src_row >= start_src_col + no_col) { // the source is completely under the main diagonal
                    unsigned src_row = start_src_row, dest_row = start_dest_row;
                    for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                        unsigned src_col = start_src_col, dest_col = start_dest_col;
                        for (unsigned j = 0; j < no_col; j++, src_col++, dest_col++) {
                            rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                        }
                    }
                }
                else { // the source is intersected by the main diagonal
                    unsigned src_row = start_src_row, dest_row = start_dest_row;
                    for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                        unsigned src_col = start_src_col, dest_col = start_dest_col;
                        for (unsigned j = 0; j < no_col; j++, src_col++, dest_col++) {
                            if (src_col >= src_row)
                                rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                            else
                                rows.unsafe(dest_row).unsafe(dest_col) = src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                        }
                    }
                }
            }
            else { // nonsyimm => nonsyimm
                for (unsigned i = 0; i < no_row; i++)
                    rows.unsafe(start_dest_row + i).subvektor_copy(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
            }
        }
    }


    //***********************************************************************
    void submatrix_plus_equal(const matrix & src, unsigned start_dest_row, unsigned start_dest_col, unsigned start_src_row, unsigned start_src_col, unsigned no_row, unsigned no_col) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no_col == 0 || no_row == 0)
            return;
        is_smaller_error(start_dest_row + no_row - 1, row,     "matrix::submatrix_plus_equal start_dest_row + no_row");
        is_smaller_error(start_src_row  + no_row - 1, src.row, "matrix::submatrix_plus_equal start_src_row  + no_row");
        if (is_symm) {
            if (start_dest_row >= start_dest_col + no_col) // i.e. start_dest_row > start_dest_col + no_col - 1
                ; // if the target falls entirely in the lower triangle, discarded
            else {
                if (src.is_symm) {  // symm => symm
                    if (start_dest_row + no_row - 1 <= start_dest_col) { // the destination is completely above the main diagonal
                        if (start_src_row + no_row - 1 <= start_src_col) { // the source is completely above the main diagonal
                            for (unsigned i = 0; i < no_row; i++)
                                rows.unsafe(start_dest_row + i).subvektor_pluszegyenlo(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
                        }
                        else if (start_src_row >= start_src_col + no_col) { // the source is completely under the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row, dest_row = start_dest_row;
                            for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                                unsigned src_col = start_src_col, dest_col = start_dest_col;
                                for (unsigned j = 0; j < no_col; j++, src_col++, dest_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                            }
                        }
                        else { // the source is intersected by the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row, dest_row = start_dest_row;
                            for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                                unsigned dest_col = start_dest_col;
                                const unsigned ig = start_src_col + no_col;
                                for (unsigned src_col = start_src_col; src_col < src_row && src_col < ig; src_col++, dest_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                                for (unsigned src_col = start_src_col > src_row ? start_src_col : src_row; src_col < ig; src_col++, dest_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                                }
                            }
                        }
                    }
                    else { // the destination is intersected by the main diagonal
                        if (start_src_row + no_row - 1 <= start_src_col) { // the source is completely above the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row;
                            for (unsigned dest_row = start_dest_row; dest_row < start_dest_row + no_row; dest_row++, src_row++) {
                                unsigned dest_col = dest_row > start_dest_col ? dest_row : start_dest_col;
                                unsigned src_col = start_src_col + dest_col - start_dest_col;
                                for (; dest_col < start_dest_col + no_col; dest_col++, src_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                                }
                            }
                        }
                        else if (start_src_row >= start_src_col + no_col) { // the source is completely under the main diagonal (this is not supposed to happen)
                            unsigned src_row = start_src_row;
                            for (unsigned dest_row = start_dest_row; dest_row < start_dest_row + no_row; dest_row++, src_row++) {
                                unsigned dest_col = dest_row > start_dest_col ? dest_row : start_dest_col;
                                unsigned src_col = start_src_col + dest_col - start_dest_col;
                                for (; dest_col < start_dest_col + no_col; dest_col++, src_col++) {
                                    rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                            }
                        }
                        else { // the source is intersected by the main diagonal
                            unsigned src_row = start_src_row;
                            for (unsigned dest_row = start_dest_row; dest_row < start_dest_row + no_row; dest_row++, src_row++) {
                                unsigned dest_col = dest_row > start_dest_col ? dest_row : start_dest_col;
                                unsigned src_col = start_src_col + dest_col - start_dest_col;
                                for (; dest_col < start_dest_col + no_col; dest_col++, src_col++) {
                                    if (src_col >= src_row)
                                        rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                                    else
                                        rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                                }
                            }
                        }
                    }
                }
                else { // nonsymm => symm (is it possible?)
                    if (start_dest_row + no_row - 1 <= start_dest_col) { // the destination is completely above the main diagonal
                        for (unsigned i = 0; i < no_row; i++)
                            rows.unsafe(start_dest_row + i).subvektor_pluszegyenlo(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
                    }
                    else { // there is also a section below the main diagonal in the destination
                        unsigned src_row = start_src_row, dest_row = start_dest_row;
                        for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                            unsigned dest_col = start_dest_col >= dest_row ? start_dest_col : dest_row;
                            unsigned src_col = start_src_col + dest_col - start_dest_col;
                            for (; dest_col < start_dest_col + no_col; src_col++, dest_col++) {
                                rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                            }
                        }
                    }
                }
            }
        }
        else {
            if (src.is_symm) { // symm => nonsymm
                if (start_src_row + no_row - 1 <= start_src_col) { // the source is completely above the main diagonal
                    for (unsigned i = 0; i < no_row; i++)
                        rows.unsafe(start_dest_row + i).subvektor_pluszegyenlo(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
                }
                else if (start_src_row >= start_src_col + no_col) { // the source is completely under the main diagonal
                    unsigned src_row = start_src_row, dest_row = start_dest_row;
                    for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                        unsigned src_col = start_src_col, dest_col = start_dest_col;
                        for (unsigned j = 0; j < no_col; j++, src_col++, dest_col++) {
                            rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                        }
                    }
                }
                else { // the source is intersected by the main diagonal
                    unsigned src_row = start_src_row, dest_row = start_dest_row;
                    for (unsigned i = 0; i < no_row; i++, src_row++, dest_row++) {
                        unsigned src_col = start_src_col, dest_col = start_dest_col;
                        for (unsigned j = 0; j < no_col; j++, src_col++, dest_col++) {
                            if (src_col >= src_row)
                                rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_row).unsafe(src_col); // dest(i,j) = src(i,j)
                            else
                                rows.unsafe(dest_row).unsafe(dest_col) += src.rows.unsafe(src_col).unsafe(src_row); // dest(i,j) = src(j,i)
                        }
                    }
                }
            }
            else { // nonsymm => nonsymm
                for (unsigned i = 0; i < no_row; i++)
                    rows.unsafe(start_dest_row + i).subvektor_pluszegyenlo(src.rows.unsafe(start_src_row + i), start_dest_col, start_src_col, no_col);
            }
        }
    }


    //***********************************************************************
    void submatrix_add(const matrix & src_1, const matrix & src_2, unsigned start_dest_row, unsigned start_dest_col, unsigned start_src_1_row, unsigned start_src_1_col, unsigned start_src_2_row, unsigned start_src_2_col, unsigned no_row, unsigned no_col) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (no_col == 0 || no_row == 0)
            return;
        is_smaller_error(start_dest_row + no_row - 1, row,         "matrix::submatrix_add start_dest_row + no_row");
        is_smaller_error(start_src_1_row  + no_row - 1, src_1.row, "matrix::submatrix_add start_src_1_row  + no_row");
        is_smaller_error(start_src_2_row  + no_row - 1, src_2.row, "matrix::submatrix_add start_src_2_row  + no_row");

        // the most common case where nothing is symmetrical

        if (!is_symm && !src_1.is_symm && !src_2.is_symm) {
            for (unsigned i = 0; i < no_row; i++)
                rows.unsafe(start_dest_row + i).subvektor_add(src_1.rows.unsafe(start_src_1_row + i), src_2.rows.unsafe(start_src_2_row + i), start_dest_col, start_src_1_col, start_src_2_col, no_col);
            return;
        }

        // Something is symmetric: it would be too complex to unpack. I hope it will be fast enough.

        submatrix_copy(src_1, start_dest_row, start_dest_col, start_src_1_row, start_src_1_col, no_row, no_col);
        submatrix_plus_equal(src_2, start_dest_row, start_dest_col, start_src_2_row, start_src_2_col, no_row, no_col);
    }


    //***********************************************************************
    friend inline void math_mul(vektor<datatype> & dest, const matrix & src1, const vektor<datatype> & src2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (dest.size() == 0)
            return;
        is_equal_error(dest.size(), src1.get_row(), "vektor math_mul size");
        is_true_error(src1.is_symm, "matrix::math_mul vektor", "symmetrical matrix not allowed");
        for (unsigned i = 0; i < dest.size(); i++)
            dest[i] = math_mul(src1.rows[i], src2);
    }


    //***********************************************************************
    friend inline void math_add_mul(vektor<datatype> & dest, const vektor<datatype> & tobeadded, const matrix & src1, const vektor<datatype> & src2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (dest.size() == 0)
            return;
        is_equal_error(dest.size(), tobeadded.size(), "vektor math_add_mul size");
        is_equal_error(dest.size(), src1.get_row(), "vektor math_mul size");
        is_true_error(src1.is_symm, "matrix::math_add_mul vektor", "symmetrical matrix not allowed");
        for (unsigned i = 0; i < dest.size(); i++)
            dest[i] = tobeadded[i] + math_mul(src1.rows[i], src2);
    }


    //***********************************************************************
    friend inline void math_add_mul_symm(vektor<datatype> & dest, const vektor<datatype> & tobeadded, const matrix & src1, const vektor<datatype> & src2) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (dest.size() == 0)
            return;
        is_equal_error(dest.size(), tobeadded.size(), "vektor math_add_mul size");
        is_equal_error(dest.size(), src1.get_row(), "vektor math_mul size");
        is_true_error(!src1.is_symm, "matrix::math_add_mul_symm vektor", "nonsymmetrical matrix not allowed");
        for (unsigned row = 0; row < dest.size(); row++) {
            datatype sum = tobeadded[row];
            for (unsigned col = 0; col < row; col++) {
                sum += src1.rows[col][row] * src2[col]; // (j,i)
            }
            for (unsigned col = row; col < dest.size(); col++) {
                sum += src1.rows[row][col] * src2[col]; // (i,j)
            }
            dest[row] = sum;
        }
    }


    //****************************************************************
    void math_ninv_np_() noexcept(!hmgVErrorCheck) {
    // without pivoting
    // Not laying-proof
    //****************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(row, col, "matrix::math_inv_np");
        is_true_error(is_symm, "matrix::math_ninv_np_", "symmetrical matrix not allowed");

        if (row == 1) {
            t[0] = datatype(-1) / t[0]; 
            return;
        }
        if (row == 2) {
            datatype * const p = &t[0];
            datatype p0 = abs(p[0]) < 1e-20f ? 1e20f : datatype(1.0f) / p[0];
            datatype p2 = -p[2] * p0;
            datatype p1 = p[1] * p0;
            datatype p3 = p[3] + p2 * p[1];
            datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
            datatype C2 = -p1 * divisor2;
            p[0] = -(p0 + C2 * p2);
            p[1] = -C2;
            p[2] = -p2 * divisor2;
            p[3] = -divisor2;
            return;
        }

        for (unsigned i = 0; i < row; i++){
            vektor<datatype> & row_i = rows[i];
            datatype divisor = abs(row_i[i]) < 1e-20f ? 1e20f : datatype(1.0f) / row_i[i];
            unsigned j;
            for (j = 0; j < i; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = row_j[i] * divisor;
                unsigned k;
                for (k = 0; k < i; k++) row_j[k] -= C * row_i[k];
                row_j[k] = C; // in case of non-negating inv = -C;
                for (k++; k < row; k++) row_j[k] -= C * row_i[k];
            }
            for (j++; j < row; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = row_j[i] * divisor;
                unsigned k;
                for (k = 0; k < i; k++) row_j[k] -= C * row_i[k];
                row_j[k] = C; // in case of non-negating inv = -C;
                for (k++; k < row; k++) row_j[k] -= C * row_i[k];
            }
            for (j = 0; j < i; j++) row_i[j] *= divisor;
            row_i[j] = -divisor; // in case of non-negating inv = divisor;
            for (j++; j < row; j++) row_i[j] *= divisor;
        }
    }

    //****************************************************************
    void math_ninv_np_blokk_2x2() noexcept(!hmgVErrorCheck) {
    // without pivoting
    //****************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(row, col, "matrix::math_ninv_np_blokk_2x2");
        is_true_error(is_symm, "matrix::math_ninv_np_blokk_2x2", "symmetrical matrix not allowed");

        if (row == 1) {
            rows[0][0] = datatype(-1) / rows[0][0];
            return;
        }
        if (row == 2) {
            math_ninv_2x2_fv(&rows[0][0], &rows[1][0]);
            return;
        }

        if (row == 4) {
            math_ninv_4x4_fv(&rows[0][0], &rows[1][0], &rows[2][0], &rows[3][0]);
            return;
        }

        const unsigned drow = row % 2;
        const unsigned hrow = row - drow;
        const unsigned nrow = row;
        for (unsigned i = 0; i < hrow; i += 2){
            vektor<datatype> & row_i0 = rows[i];
            vektor<datatype> & row_i1 = rows[i + 1];
            
            // (i,i)-nél lévõ elem inverze
            const datatype p0 = abs(row_i0[i]) < 1e-20f ? 1e20f : datatype(1.0f) / row_i0[i];
            const datatype p1o = row_i0[i + 1];
            const datatype p1 = p1o * p0;
            const datatype p2 = -row_i1[i] * p0;
            const datatype p3 = row_i1.unsafe(i+1) + p2 * p1o;
            const datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
            const datatype C2 = -p1 * divisor2;
            const datatype a0 = p0 + C2 * p2; // a0...a3 are the inverse
            const datatype a1 = C2;
            const datatype a2 = p2 * divisor2;
            const datatype a3 = divisor2;

            unsigned j;
            for (j = 0; j < i; j += 2) {
                vektor<datatype> & row_j0 = rows[j];
                vektor<datatype> & row_j1 = rows[j + 1];
                const datatype C0 = row_j0[i] * a0 + row_j0[i + 1] * a2;
                const datatype C1 = row_j0[i] * a1 + row_j0[i + 1] * a3;
                const datatype C2 = row_j1[i] * a0 + row_j1[i + 1] * a2;
                const datatype C3 = row_j1[i] * a1 + row_j1[i + 1] * a3;
                unsigned k;
                for (k = 0; k < i; k += 2) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j0[k + 1] -= C0 * row_i0[k + 1] + C1 * row_i1[k + 1];
                    row_j1[k]     -= C2 * row_i0[k]     + C3 * row_i1[k];
                    row_j1[k + 1] -= C2 * row_i0[k + 1] + C3 * row_i1[k + 1];
                }
                row_j0[k]     = C0; // in case of non-negating inv = -C;
                row_j0[k + 1] = C1;
                row_j1[k]     = C2;
                row_j1[k + 1] = C3;
                for (k += 2; k < hrow; k += 2) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j0[k + 1] -= C0 * row_i0[k + 1] + C1 * row_i1[k + 1];
                    row_j1[k]     -= C2 * row_i0[k]     + C3 * row_i1[k];
                    row_j1[k + 1] -= C2 * row_i0[k + 1] + C3 * row_i1[k + 1];
                }
                for (k = hrow; k < nrow; k++) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j1[k]     -= C2 * row_i0[k]     + C3 * row_i1[k];
                }
            }
            for (j+=2; j < hrow; j += 2) {
                vektor<datatype> & row_j0 = rows[j];
                vektor<datatype> & row_j1 = rows[j + 1];
                const datatype C0 = row_j0[i] * a0 + row_j0[i + 1] * a2;
                const datatype C1 = row_j0[i] * a1 + row_j0[i + 1] * a3;
                const datatype C2 = row_j1[i] * a0 + row_j1[i + 1] * a2;
                const datatype C3 = row_j1[i] * a1 + row_j1[i + 1] * a3;
                unsigned k;
                for (k = 0; k < i; k += 2) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j0[k + 1] -= C0 * row_i0[k + 1] + C1 * row_i1[k + 1];
                    row_j1[k]     -= C2 * row_i0[k]     + C3 * row_i1[k];
                    row_j1[k + 1] -= C2 * row_i0[k + 1] + C3 * row_i1[k + 1];
                }
                row_j0[k]     = C0; // in case of non-negating inv = -C;
                row_j0[k + 1] = C1;
                row_j1[k]     = C2;
                row_j1[k + 1] = C3;
                for (k += 2; k < hrow; k += 2) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j0[k + 1] -= C0 * row_i0[k + 1] + C1 * row_i1[k + 1];
                    row_j1[k]     -= C2 * row_i0[k]     + C3 * row_i1[k];
                    row_j1[k + 1] -= C2 * row_i0[k + 1] + C3 * row_i1[k + 1];
                }
                for (k = hrow; k < nrow; k++) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j1[k]     -= C2 * row_i0[k]     + C3 * row_i1[k];
                }
            }
            for (j = hrow; j < nrow; j++) {
                vektor<datatype> & row_j0 = rows[j];
                const datatype C0 = row_j0[i] * a0 + row_j0[i + 1] * a2;
                const datatype C1 = row_j0[i] * a1 + row_j0[i + 1] * a3;
                unsigned k;
                for (k = 0; k < i; k += 2) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j0[k + 1] -= C0 * row_i0[k + 1] + C1 * row_i1[k + 1];
                }
                row_j0[k]     = C0; // in case of non-negating inv = -C;
                row_j0[k + 1] = C1;
                for (k += 2; k < hrow; k += 2) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                    row_j0[k + 1] -= C0 * row_i0[k + 1] + C1 * row_i1[k + 1];
                }
                for (k = hrow; k < nrow; k++) {
                    row_j0[k]     -= C0 * row_i0[k]     + C1 * row_i1[k];
                }
            }
            for (j = 0; j < i; j += 2) {
                const datatype b0 = a0 * row_i0[j]     + a1 * row_i1[j];
                const datatype b1 = a0 * row_i0[j + 1] + a1 * row_i1[j + 1];
                const datatype b2 = a2 * row_i0[j]     + a3 * row_i1[j];
                const datatype b3 = a2 * row_i0[j + 1] + a3 * row_i1[j + 1];
                row_i0[j]     = b0;
                row_i0[j + 1] = b1;
                row_i1[j]     = b2;
                row_i1[j + 1] = b3;
            }
            row_i0[j]     = -a0; // in case of non-negating inv = ai;
            row_i0[j + 1] = -a1;
            row_i1[j]     = -a2;
            row_i1[j + 1] = -a3;
            for (j += 2; j < hrow; j += 2) {
                const datatype b0 = a0 * row_i0[j]     + a1 * row_i1[j];
                const datatype b1 = a0 * row_i0[j + 1] + a1 * row_i1[j + 1];
                const datatype b2 = a2 * row_i0[j]     + a3 * row_i1[j];
                const datatype b3 = a2 * row_i0[j + 1] + a3 * row_i1[j + 1];
                row_i0[j]     = b0;
                row_i0[j + 1] = b1;
                row_i1[j]     = b2;
                row_i1[j + 1] = b3;
            }
            for (j = hrow; j < nrow; j++) {
                const datatype b0 = a0 * row_i0[j]     + a1 * row_i1[j];
                const datatype b2 = a2 * row_i0[j]     + a3 * row_i1[j];
                row_i0[j]     = b0;
                row_i1[j]     = b2;
            }
        }
        for (unsigned i = hrow; i < nrow; i++) {
            vektor<datatype> & row_i = rows[i];
            datatype divisor = abs(row_i[i]) < 1e-20f ? 1e20f : datatype(1.0f) / row_i[i];
            unsigned j;
            for (j = 0; j < i; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = row_j[i] * divisor;
                unsigned k;
                for (k = 0; k < i; k++) row_j[k] -= C * row_i[k];
                row_j[k] = C; // in case of non-negating inv = -C;
                for (k++; k < row; k++) row_j[k] -= C * row_i[k];
            }
            for (j++; j < row; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = row_j[i] * divisor;
                unsigned k;
                for (k = 0; k < i; k++) row_j[k] -= C * row_i[k];
                row_j[k] = C; // in case of non-negating inv = -C;
                for (k++; k < row; k++) row_j[k] -= C * row_i[k];
            }
            for (j = 0; j < i; j++) row_i[j] *= divisor;
            row_i[j] = -divisor; // in case of non-negating inv = divisor;
            for (j++; j < row; j++) row_i[j] *= divisor;
        }
    }

    //****************************************************************
    static inline void math_ninv_2x2_fv(datatype * in0, datatype * in1) noexcept {
    // invert one 2x2 matrix, input the address of the first element of the two lines
    //****************************************************************
        datatype p0 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        datatype p2 = -in1[0] * p0;
        datatype p1 = in0[1] * p0;
        datatype p3 = in1[1] + p2 * in0[1];
        datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
        datatype C2 = -p1 * divisor2;
        in0[0] = -(p0 + C2 * p2);
        in0[1] = -C2;
        in1[0] = -p2 * divisor2;
        in1[1] = -divisor2;
    }

    //****************************************************************
    static inline void math_inv_2x2_fv(datatype * in0, datatype * in1) noexcept {
    // invert one 2x2 matrix, input the address of the first element of the two lines
    //****************************************************************
        datatype p0 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        datatype p2 = -in1[0] * p0;
        datatype p1 = in0[1] * p0;
        datatype p3 = in1[1] + p2 * in0[1];
        datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
        datatype C2 = -p1 * divisor2;
        in0[0] = p0 + C2 * p2;
        in0[1] = C2;
        in1[0] = p2 * divisor2;
        in1[1] = divisor2;
    }

    //****************************************************************
    static inline void math_inv_4x4_fv(datatype * in0, datatype * in1, datatype * in2, datatype * in3) noexcept {
    // invert one 4x4 matrix, input the address of the first element of the four lines
    //****************************************************************
        // 0th row
        datatype a00 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        datatype a01 = a00 * in0[1];
        datatype a02 = a00 * in0[2];
        datatype a03 = a00 * in0[3];
        datatype a10 =        - in1[0] * a00;
        datatype a11 = in1[1] - in1[0] * a01;
        datatype a12 = in1[2] - in1[0] * a02;
        datatype a13 = in1[3] - in1[0] * a03;
        datatype a20 =        - in2[0] * a00;
        datatype a21 = in2[1] - in2[0] * a01;
        datatype a22 = in2[2] - in2[0] * a02;
        datatype a23 = in2[3] - in2[0] * a03;
        datatype a30 =        - in3[0] * a00;
        datatype a31 = in3[1] - in3[0] * a01;
        datatype a32 = in3[2] - in3[0] * a02;
        datatype a33 = in3[3] - in3[0] * a03;
        // 1st row
        a11 = abs(a11) < 1e-20f ? 1e20f : datatype(1.0f) / a11;
        a10 *= a11;
        a12 *= a11;
        a13 *= a11;
        a00 -= a01 * a10;
        a02 -= a01 * a12;
        a03 -= a01 * a13;
        a01 *= -a11;
        a20 -= a21 * a10;
        a22 -= a21 * a12;
        a23 -= a21 * a13;
        a21 *= -a11;
        a30 -= a31 * a10;
        a32 -= a31 * a12;
        a33 -= a31 * a13;
        a31 *= -a11;
        // 2nd row
        a22 = abs(a22) < 1e-20f ? 1e20f : datatype(1.0f) / a22;
        a20 *= a22;
        a21 *= a22;
        a23 *= a22;
        a00 -= a02 * a20;
        a01 -= a02 * a21;
        a03 -= a02 * a23;
        a02 *= -a22;
        a10 -= a12 * a20;
        a11 -= a12 * a21;
        a13 -= a12 * a23;
        a12 *= -a22;
        a30 -= a32 * a20;
        a31 -= a32 * a21;
        a33 -= a32 * a23;
        a32 *= -a22;
        // 3rd row
        a33 = abs(a33) < 1e-20f ? 1e20f : datatype(1.0f) / a33;
        a30 *= a33;
        a31 *= a33;
        a32 *= a33;
        in0[0] = a00 - a03 * a30;
        in0[1] = a01 - a03 * a31;
        in0[2] = a02 - a03 * a32;
        in0[3] = -a03 * a33;
        in1[0] = a10 - a13 * a30;
        in1[1] = a11 - a13 * a31;
        in1[2] = a12 - a13 * a32;
        in1[3] = -a13 * a33;
        in2[0] = a20 - a23 * a30;
        in2[1] = a21 - a23 * a31;
        in2[2] = a22 - a23 * a32;
        in2[3] = -a23 * a33;
        in3[0] = a30;
        in3[1] = a31;
        in3[2] = a32;
        in3[3] = a33;
    }

    //****************************************************************
    static inline void math_ninv_4x4_fv(datatype * in0, datatype * in1, datatype * in2, datatype * in3) noexcept {
    // invert one 4x4 matrix, input the address of the first element of the four lines
    //****************************************************************
        // 0th row
        datatype a00 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        datatype a01 = a00 * in0[1];
        datatype a02 = a00 * in0[2];
        datatype a03 = a00 * in0[3];
        datatype a10 =        - in1[0] * a00;
        datatype a11 = in1[1] - in1[0] * a01;
        datatype a12 = in1[2] - in1[0] * a02;
        datatype a13 = in1[3] - in1[0] * a03;
        datatype a20 =        - in2[0] * a00;
        datatype a21 = in2[1] - in2[0] * a01;
        datatype a22 = in2[2] - in2[0] * a02;
        datatype a23 = in2[3] - in2[0] * a03;
        datatype a30 =        - in3[0] * a00;
        datatype a31 = in3[1] - in3[0] * a01;
        datatype a32 = in3[2] - in3[0] * a02;
        datatype a33 = in3[3] - in3[0] * a03;
        // 1st row
        a11 = abs(a11) < 1e-20f ? 1e20f : datatype(1.0f) / a11;
        a10 *= a11;
        a12 *= a11;
        a13 *= a11;
        a00 -= a01 * a10;
        a02 -= a01 * a12;
        a03 -= a01 * a13;
        a01 *= -a11;
        a20 -= a21 * a10;
        a22 -= a21 * a12;
        a23 -= a21 * a13;
        a21 *= -a11;
        a30 -= a31 * a10;
        a32 -= a31 * a12;
        a33 -= a31 * a13;
        a31 *= -a11;
        // 2nd row
        a22 = abs(a22) < 1e-20f ? 1e20f : datatype(1.0f) / a22;
        a20 *= a22;
        a21 *= a22;
        a23 *= a22;
        a00 -= a02 * a20;
        a01 -= a02 * a21;
        a03 -= a02 * a23;
        a02 *= -a22;
        a10 -= a12 * a20;
        a11 -= a12 * a21;
        a13 -= a12 * a23;
        a12 *= -a22;
        a30 -= a32 * a20;
        a31 -= a32 * a21;
        a33 -= a32 * a23;
        a32 *= -a22;
        // 3rd row
        a33 = abs(a33) < 1e-20f ? 1e20f : datatype(1.0f) / a33;
        a30 *= a33;
        a31 *= a33;
        a32 *= a33;
        in0[0] = a03 * a30 - a00;
        in0[1] = a03 * a31 - a01;
        in0[2] = a03 * a32 - a02;
        in0[3] = a03 * a33;
        in1[0] = a13 * a30 - a10;
        in1[1] = a13 * a31 - a11;
        in1[2] = a13 * a32 - a12;
        in1[3] = a13 * a33;
        in2[0] = a23 * a30 - a20;
        in2[1] = a23 * a31 - a21;
        in2[2] = a23 * a32 - a22;
        in2[3] = a23 * a33;
        in3[0] = -a30;
        in3[1] = -a31;
        in3[2] = -a32;
        in3[3] = -a33;
    }

    //****************************************************************
    void math_ninv_np/*_blokk_4x4*/() noexcept(!hmgVErrorCheck) {
    // without pivoting
    //****************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(row, col, "matrix::math_ninv_np");
        is_true_error(is_symm, "matrix::math_ninv_np", "symmetrical matrix not allowed");

        if (row == 1) {
            rows[0][0] = datatype(-1) / rows[0][0];
            return;
        }
        if (row == 2) {
            math_ninv_2x2_fv(&rows[0][0], &rows[1][0]);
            return;
        }
        if (row == 4) {
            math_ninv_4x4_fv(&rows[0][0], &rows[1][0], &rows[2][0], &rows[3][0]);
            return;
        }

        const unsigned drow = row % 4;
        const unsigned hrow = row - drow;
        const unsigned nrow = row;
        for (unsigned i = 0; i < hrow; i += 4){
            vektor<datatype> & row_i0 = rows[i];
            vektor<datatype> & row_i1 = rows[i + 1];
            vektor<datatype> & row_i2 = rows[i + 2];
            vektor<datatype> & row_i3 = rows[i + 3];

            // inverse of the element at (i,i)
            datatype a0[4], a1[4], a2[4], a3[4];
            a0[0] = row_i0[i];    a0[1] = row_i0[i + 1];    a0[2] = row_i0[i + 2];    a0[3] = row_i0[i + 3];
            a1[0] = row_i1[i];    a1[1] = row_i1[i + 1];    a1[2] = row_i1[i + 2];    a1[3] = row_i1[i + 3];
            a2[0] = row_i2[i];    a2[1] = row_i2[i + 1];    a2[2] = row_i2[i + 2];    a2[3] = row_i2[i + 3];
            a3[0] = row_i3[i];    a3[1] = row_i3[i + 1];    a3[2] = row_i3[i + 2];    a3[3] = row_i3[i + 3];
            math_inv_4x4_fv(a0, a1, a2, a3); // a0...a3 are the inverse

            unsigned j;
            for (j = 0; j < i; j += 4) {
                vektor<datatype> & row_j0 = rows[j];
                vektor<datatype> & row_j1 = rows[j + 1];
                vektor<datatype> & row_j2 = rows[j + 2];
                vektor<datatype> & row_j3 = rows[j + 3];
                const datatype C00 = row_j0[i] * a0[0] + row_j0[i + 1] * a1[0] + row_j0[i + 2] * a2[0] + row_j0[i + 3] * a3[0];
                const datatype C01 = row_j0[i] * a0[1] + row_j0[i + 1] * a1[1] + row_j0[i + 2] * a2[1] + row_j0[i + 3] * a3[1];
                const datatype C02 = row_j0[i] * a0[2] + row_j0[i + 1] * a1[2] + row_j0[i + 2] * a2[2] + row_j0[i + 3] * a3[2];
                const datatype C03 = row_j0[i] * a0[3] + row_j0[i + 1] * a1[3] + row_j0[i + 2] * a2[3] + row_j0[i + 3] * a3[3];
                const datatype C10 = row_j1[i] * a0[0] + row_j1[i + 1] * a1[0] + row_j1[i + 2] * a2[0] + row_j1[i + 3] * a3[0];
                const datatype C11 = row_j1[i] * a0[1] + row_j1[i + 1] * a1[1] + row_j1[i + 2] * a2[1] + row_j1[i + 3] * a3[1];
                const datatype C12 = row_j1[i] * a0[2] + row_j1[i + 1] * a1[2] + row_j1[i + 2] * a2[2] + row_j1[i + 3] * a3[2];
                const datatype C13 = row_j1[i] * a0[3] + row_j1[i + 1] * a1[3] + row_j1[i + 2] * a2[3] + row_j1[i + 3] * a3[3];
                const datatype C20 = row_j2[i] * a0[0] + row_j2[i + 1] * a1[0] + row_j2[i + 2] * a2[0] + row_j2[i + 3] * a3[0];
                const datatype C21 = row_j2[i] * a0[1] + row_j2[i + 1] * a1[1] + row_j2[i + 2] * a2[1] + row_j2[i + 3] * a3[1];
                const datatype C22 = row_j2[i] * a0[2] + row_j2[i + 1] * a1[2] + row_j2[i + 2] * a2[2] + row_j2[i + 3] * a3[2];
                const datatype C23 = row_j2[i] * a0[3] + row_j2[i + 1] * a1[3] + row_j2[i + 2] * a2[3] + row_j2[i + 3] * a3[3];
                const datatype C30 = row_j3[i] * a0[0] + row_j3[i + 1] * a1[0] + row_j3[i + 2] * a2[0] + row_j3[i + 3] * a3[0];
                const datatype C31 = row_j3[i] * a0[1] + row_j3[i + 1] * a1[1] + row_j3[i + 2] * a2[1] + row_j3[i + 3] * a3[1];
                const datatype C32 = row_j3[i] * a0[2] + row_j3[i + 1] * a1[2] + row_j3[i + 2] * a2[2] + row_j3[i + 3] * a3[2];
                const datatype C33 = row_j3[i] * a0[3] + row_j3[i + 1] * a1[3] + row_j3[i + 2] * a2[3] + row_j3[i + 3] * a3[3];
                unsigned k;
                for (k = 0; k < i; k += 4) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j0[k + 1] -= C00 * row_i0[k + 1] + C01 * row_i1[k + 1] + C02 * row_i2[k + 1] + C03 * row_i3[k + 1];
                    row_j0[k + 2] -= C00 * row_i0[k + 2] + C01 * row_i1[k + 2] + C02 * row_i2[k + 2] + C03 * row_i3[k + 2];
                    row_j0[k + 3] -= C00 * row_i0[k + 3] + C01 * row_i1[k + 3] + C02 * row_i2[k + 3] + C03 * row_i3[k + 3];
                    row_j1[k]     -= C10 * row_i0[k]     + C11 * row_i1[k]     + C12 * row_i2[k]     + C13 * row_i3[k];
                    row_j1[k + 1] -= C10 * row_i0[k + 1] + C11 * row_i1[k + 1] + C12 * row_i2[k + 1] + C13 * row_i3[k + 1];
                    row_j1[k + 2] -= C10 * row_i0[k + 2] + C11 * row_i1[k + 2] + C12 * row_i2[k + 2] + C13 * row_i3[k + 2];
                    row_j1[k + 3] -= C10 * row_i0[k + 3] + C11 * row_i1[k + 3] + C12 * row_i2[k + 3] + C13 * row_i3[k + 3];
                    row_j2[k]     -= C20 * row_i0[k]     + C21 * row_i1[k]     + C22 * row_i2[k]     + C23 * row_i3[k];
                    row_j2[k + 1] -= C20 * row_i0[k + 1] + C21 * row_i1[k + 1] + C22 * row_i2[k + 1] + C23 * row_i3[k + 1];
                    row_j2[k + 2] -= C20 * row_i0[k + 2] + C21 * row_i1[k + 2] + C22 * row_i2[k + 2] + C23 * row_i3[k + 2];
                    row_j2[k + 3] -= C20 * row_i0[k + 3] + C21 * row_i1[k + 3] + C22 * row_i2[k + 3] + C23 * row_i3[k + 3];
                    row_j3[k]     -= C30 * row_i0[k]     + C31 * row_i1[k]     + C32 * row_i2[k]     + C33 * row_i3[k];
                    row_j3[k + 1] -= C30 * row_i0[k + 1] + C31 * row_i1[k + 1] + C32 * row_i2[k + 1] + C33 * row_i3[k + 1];
                    row_j3[k + 2] -= C30 * row_i0[k + 2] + C31 * row_i1[k + 2] + C32 * row_i2[k + 2] + C33 * row_i3[k + 2];
                    row_j3[k + 3] -= C30 * row_i0[k + 3] + C31 * row_i1[k + 3] + C32 * row_i2[k + 3] + C33 * row_i3[k + 3];
                }
                row_j0[k]     = C00;    row_j0[k + 1] = C01;    row_j0[k + 2] = C02;    row_j0[k + 3] = C03; // in case of non-negating inv = -C;
                row_j1[k]     = C10;    row_j1[k + 1] = C11;    row_j1[k + 2] = C12;    row_j1[k + 3] = C13;
                row_j2[k]     = C20;    row_j2[k + 1] = C21;    row_j2[k + 2] = C22;    row_j2[k + 3] = C23;
                row_j3[k]     = C30;    row_j3[k + 1] = C31;    row_j3[k + 2] = C32;    row_j3[k + 3] = C33;
                for (k += 4; k < hrow; k += 4) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j0[k + 1] -= C00 * row_i0[k + 1] + C01 * row_i1[k + 1] + C02 * row_i2[k + 1] + C03 * row_i3[k + 1];
                    row_j0[k + 2] -= C00 * row_i0[k + 2] + C01 * row_i1[k + 2] + C02 * row_i2[k + 2] + C03 * row_i3[k + 2];
                    row_j0[k + 3] -= C00 * row_i0[k + 3] + C01 * row_i1[k + 3] + C02 * row_i2[k + 3] + C03 * row_i3[k + 3];
                    row_j1[k]     -= C10 * row_i0[k]     + C11 * row_i1[k]     + C12 * row_i2[k]     + C13 * row_i3[k];
                    row_j1[k + 1] -= C10 * row_i0[k + 1] + C11 * row_i1[k + 1] + C12 * row_i2[k + 1] + C13 * row_i3[k + 1];
                    row_j1[k + 2] -= C10 * row_i0[k + 2] + C11 * row_i1[k + 2] + C12 * row_i2[k + 2] + C13 * row_i3[k + 2];
                    row_j1[k + 3] -= C10 * row_i0[k + 3] + C11 * row_i1[k + 3] + C12 * row_i2[k + 3] + C13 * row_i3[k + 3];
                    row_j2[k]     -= C20 * row_i0[k]     + C21 * row_i1[k]     + C22 * row_i2[k]     + C23 * row_i3[k];
                    row_j2[k + 1] -= C20 * row_i0[k + 1] + C21 * row_i1[k + 1] + C22 * row_i2[k + 1] + C23 * row_i3[k + 1];
                    row_j2[k + 2] -= C20 * row_i0[k + 2] + C21 * row_i1[k + 2] + C22 * row_i2[k + 2] + C23 * row_i3[k + 2];
                    row_j2[k + 3] -= C20 * row_i0[k + 3] + C21 * row_i1[k + 3] + C22 * row_i2[k + 3] + C23 * row_i3[k + 3];
                    row_j3[k]     -= C30 * row_i0[k]     + C31 * row_i1[k]     + C32 * row_i2[k]     + C33 * row_i3[k];
                    row_j3[k + 1] -= C30 * row_i0[k + 1] + C31 * row_i1[k + 1] + C32 * row_i2[k + 1] + C33 * row_i3[k + 1];
                    row_j3[k + 2] -= C30 * row_i0[k + 2] + C31 * row_i1[k + 2] + C32 * row_i2[k + 2] + C33 * row_i3[k + 2];
                    row_j3[k + 3] -= C30 * row_i0[k + 3] + C31 * row_i1[k + 3] + C32 * row_i2[k + 3] + C33 * row_i3[k + 3];
                }
                for (k = hrow; k < nrow; k++) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j1[k]     -= C10 * row_i0[k]     + C11 * row_i1[k]     + C12 * row_i2[k]     + C13 * row_i3[k];
                    row_j2[k]     -= C20 * row_i0[k]     + C21 * row_i1[k]     + C22 * row_i2[k]     + C23 * row_i3[k];
                    row_j3[k]     -= C30 * row_i0[k]     + C31 * row_i1[k]     + C32 * row_i2[k]     + C33 * row_i3[k];
                }
            }
            for (j+=4; j < hrow; j += 4) {
                vektor<datatype> & row_j0 = rows[j];
                vektor<datatype> & row_j1 = rows[j + 1];
                vektor<datatype> & row_j2 = rows[j + 2];
                vektor<datatype> & row_j3 = rows[j + 3];
                const datatype C00 = row_j0[i] * a0[0] + row_j0[i + 1] * a1[0] + row_j0[i + 2] * a2[0] + row_j0[i + 3] * a3[0];
                const datatype C01 = row_j0[i] * a0[1] + row_j0[i + 1] * a1[1] + row_j0[i + 2] * a2[1] + row_j0[i + 3] * a3[1];
                const datatype C02 = row_j0[i] * a0[2] + row_j0[i + 1] * a1[2] + row_j0[i + 2] * a2[2] + row_j0[i + 3] * a3[2];
                const datatype C03 = row_j0[i] * a0[3] + row_j0[i + 1] * a1[3] + row_j0[i + 2] * a2[3] + row_j0[i + 3] * a3[3];
                const datatype C10 = row_j1[i] * a0[0] + row_j1[i + 1] * a1[0] + row_j1[i + 2] * a2[0] + row_j1[i + 3] * a3[0];
                const datatype C11 = row_j1[i] * a0[1] + row_j1[i + 1] * a1[1] + row_j1[i + 2] * a2[1] + row_j1[i + 3] * a3[1];
                const datatype C12 = row_j1[i] * a0[2] + row_j1[i + 1] * a1[2] + row_j1[i + 2] * a2[2] + row_j1[i + 3] * a3[2];
                const datatype C13 = row_j1[i] * a0[3] + row_j1[i + 1] * a1[3] + row_j1[i + 2] * a2[3] + row_j1[i + 3] * a3[3];
                const datatype C20 = row_j2[i] * a0[0] + row_j2[i + 1] * a1[0] + row_j2[i + 2] * a2[0] + row_j2[i + 3] * a3[0];
                const datatype C21 = row_j2[i] * a0[1] + row_j2[i + 1] * a1[1] + row_j2[i + 2] * a2[1] + row_j2[i + 3] * a3[1];
                const datatype C22 = row_j2[i] * a0[2] + row_j2[i + 1] * a1[2] + row_j2[i + 2] * a2[2] + row_j2[i + 3] * a3[2];
                const datatype C23 = row_j2[i] * a0[3] + row_j2[i + 1] * a1[3] + row_j2[i + 2] * a2[3] + row_j2[i + 3] * a3[3];
                const datatype C30 = row_j3[i] * a0[0] + row_j3[i + 1] * a1[0] + row_j3[i + 2] * a2[0] + row_j3[i + 3] * a3[0];
                const datatype C31 = row_j3[i] * a0[1] + row_j3[i + 1] * a1[1] + row_j3[i + 2] * a2[1] + row_j3[i + 3] * a3[1];
                const datatype C32 = row_j3[i] * a0[2] + row_j3[i + 1] * a1[2] + row_j3[i + 2] * a2[2] + row_j3[i + 3] * a3[2];
                const datatype C33 = row_j3[i] * a0[3] + row_j3[i + 1] * a1[3] + row_j3[i + 2] * a2[3] + row_j3[i + 3] * a3[3];
                unsigned k;
                for (k = 0; k < i; k += 4) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j0[k + 1] -= C00 * row_i0[k + 1] + C01 * row_i1[k + 1] + C02 * row_i2[k + 1] + C03 * row_i3[k + 1];
                    row_j0[k + 2] -= C00 * row_i0[k + 2] + C01 * row_i1[k + 2] + C02 * row_i2[k + 2] + C03 * row_i3[k + 2];
                    row_j0[k + 3] -= C00 * row_i0[k + 3] + C01 * row_i1[k + 3] + C02 * row_i2[k + 3] + C03 * row_i3[k + 3];
                    row_j1[k]     -= C10 * row_i0[k]     + C11 * row_i1[k]     + C12 * row_i2[k]     + C13 * row_i3[k];
                    row_j1[k + 1] -= C10 * row_i0[k + 1] + C11 * row_i1[k + 1] + C12 * row_i2[k + 1] + C13 * row_i3[k + 1];
                    row_j1[k + 2] -= C10 * row_i0[k + 2] + C11 * row_i1[k + 2] + C12 * row_i2[k + 2] + C13 * row_i3[k + 2];
                    row_j1[k + 3] -= C10 * row_i0[k + 3] + C11 * row_i1[k + 3] + C12 * row_i2[k + 3] + C13 * row_i3[k + 3];
                    row_j2[k]     -= C20 * row_i0[k]     + C21 * row_i1[k]     + C22 * row_i2[k]     + C23 * row_i3[k];
                    row_j2[k + 1] -= C20 * row_i0[k + 1] + C21 * row_i1[k + 1] + C22 * row_i2[k + 1] + C23 * row_i3[k + 1];
                    row_j2[k + 2] -= C20 * row_i0[k + 2] + C21 * row_i1[k + 2] + C22 * row_i2[k + 2] + C23 * row_i3[k + 2];
                    row_j2[k + 3] -= C20 * row_i0[k + 3] + C21 * row_i1[k + 3] + C22 * row_i2[k + 3] + C23 * row_i3[k + 3];
                    row_j3[k]     -= C30 * row_i0[k]     + C31 * row_i1[k]     + C32 * row_i2[k]     + C33 * row_i3[k];
                    row_j3[k + 1] -= C30 * row_i0[k + 1] + C31 * row_i1[k + 1] + C32 * row_i2[k + 1] + C33 * row_i3[k + 1];
                    row_j3[k + 2] -= C30 * row_i0[k + 2] + C31 * row_i1[k + 2] + C32 * row_i2[k + 2] + C33 * row_i3[k + 2];
                    row_j3[k + 3] -= C30 * row_i0[k + 3] + C31 * row_i1[k + 3] + C32 * row_i2[k + 3] + C33 * row_i3[k + 3];
                }
                row_j0[k]     = C00;    row_j0[k + 1] = C01;    row_j0[k + 2] = C02;    row_j0[k + 3] = C03; // in case of non-negating inv = -C;
                row_j1[k]     = C10;    row_j1[k + 1] = C11;    row_j1[k + 2] = C12;    row_j1[k + 3] = C13;
                row_j2[k]     = C20;    row_j2[k + 1] = C21;    row_j2[k + 2] = C22;    row_j2[k + 3] = C23;
                row_j3[k]     = C30;    row_j3[k + 1] = C31;    row_j3[k + 2] = C32;    row_j3[k + 3] = C33;
                for (k += 4; k < hrow; k += 4) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j0[k + 1] -= C00 * row_i0[k + 1] + C01 * row_i1[k + 1] + C02 * row_i2[k + 1] + C03 * row_i3[k + 1];
                    row_j0[k + 2] -= C00 * row_i0[k + 2] + C01 * row_i1[k + 2] + C02 * row_i2[k + 2] + C03 * row_i3[k + 2];
                    row_j0[k + 3] -= C00 * row_i0[k + 3] + C01 * row_i1[k + 3] + C02 * row_i2[k + 3] + C03 * row_i3[k + 3];
                    row_j1[k]     -= C10 * row_i0[k]     + C11 * row_i1[k]     + C12 * row_i2[k]     + C13 * row_i3[k];
                    row_j1[k + 1] -= C10 * row_i0[k + 1] + C11 * row_i1[k + 1] + C12 * row_i2[k + 1] + C13 * row_i3[k + 1];
                    row_j1[k + 2] -= C10 * row_i0[k + 2] + C11 * row_i1[k + 2] + C12 * row_i2[k + 2] + C13 * row_i3[k + 2];
                    row_j1[k + 3] -= C10 * row_i0[k + 3] + C11 * row_i1[k + 3] + C12 * row_i2[k + 3] + C13 * row_i3[k + 3];
                    row_j2[k]     -= C20 * row_i0[k]     + C21 * row_i1[k]     + C22 * row_i2[k]     + C23 * row_i3[k];
                    row_j2[k + 1] -= C20 * row_i0[k + 1] + C21 * row_i1[k + 1] + C22 * row_i2[k + 1] + C23 * row_i3[k + 1];
                    row_j2[k + 2] -= C20 * row_i0[k + 2] + C21 * row_i1[k + 2] + C22 * row_i2[k + 2] + C23 * row_i3[k + 2];
                    row_j2[k + 3] -= C20 * row_i0[k + 3] + C21 * row_i1[k + 3] + C22 * row_i2[k + 3] + C23 * row_i3[k + 3];
                    row_j3[k]     -= C30 * row_i0[k]     + C31 * row_i1[k]     + C32 * row_i2[k]     + C33 * row_i3[k];
                    row_j3[k + 1] -= C30 * row_i0[k + 1] + C31 * row_i1[k + 1] + C32 * row_i2[k + 1] + C33 * row_i3[k + 1];
                    row_j3[k + 2] -= C30 * row_i0[k + 2] + C31 * row_i1[k + 2] + C32 * row_i2[k + 2] + C33 * row_i3[k + 2];
                    row_j3[k + 3] -= C30 * row_i0[k + 3] + C31 * row_i1[k + 3] + C32 * row_i2[k + 3] + C33 * row_i3[k + 3];
                }
                for (k = hrow; k < nrow; k++) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j1[k]     -= C10 * row_i0[k]     + C11 * row_i1[k]     + C12 * row_i2[k]     + C13 * row_i3[k];
                    row_j2[k]     -= C20 * row_i0[k]     + C21 * row_i1[k]     + C22 * row_i2[k]     + C23 * row_i3[k];
                    row_j3[k]     -= C30 * row_i0[k]     + C31 * row_i1[k]     + C32 * row_i2[k]     + C33 * row_i3[k];
                }
            }
            for (j = hrow; j < nrow; j++) {
                vektor<datatype> & row_j0 = rows[j];
                const datatype C00 = row_j0[i] * a0[0] + row_j0[i + 1] * a1[0] + row_j0[i + 2] * a2[0] + row_j0[i + 3] * a3[0];
                const datatype C01 = row_j0[i] * a0[1] + row_j0[i + 1] * a1[1] + row_j0[i + 2] * a2[1] + row_j0[i + 3] * a3[1];
                const datatype C02 = row_j0[i] * a0[2] + row_j0[i + 1] * a1[2] + row_j0[i + 2] * a2[2] + row_j0[i + 3] * a3[2];
                const datatype C03 = row_j0[i] * a0[3] + row_j0[i + 1] * a1[3] + row_j0[i + 2] * a2[3] + row_j0[i + 3] * a3[3];
                unsigned k;
                for (k = 0; k < i; k += 4) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j0[k + 1] -= C00 * row_i0[k + 1] + C01 * row_i1[k + 1] + C02 * row_i2[k + 1] + C03 * row_i3[k + 1];
                    row_j0[k + 2] -= C00 * row_i0[k + 2] + C01 * row_i1[k + 2] + C02 * row_i2[k + 2] + C03 * row_i3[k + 2];
                    row_j0[k + 3] -= C00 * row_i0[k + 3] + C01 * row_i1[k + 3] + C02 * row_i2[k + 3] + C03 * row_i3[k + 3];
                }
                row_j0[k] = C00;    row_j0[k + 1] = C01;    row_j0[k + 2] = C02;    row_j0[k + 3] = C03; // in case of non-negating inv = -C;
                for (k += 4; k < hrow; k += 4) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                    row_j0[k + 1] -= C00 * row_i0[k + 1] + C01 * row_i1[k + 1] + C02 * row_i2[k + 1] + C03 * row_i3[k + 1];
                    row_j0[k + 2] -= C00 * row_i0[k + 2] + C01 * row_i1[k + 2] + C02 * row_i2[k + 2] + C03 * row_i3[k + 2];
                    row_j0[k + 3] -= C00 * row_i0[k + 3] + C01 * row_i1[k + 3] + C02 * row_i2[k + 3] + C03 * row_i3[k + 3];
                }
                for (k = hrow; k < nrow; k++) {
                    row_j0[k]     -= C00 * row_i0[k]     + C01 * row_i1[k]     + C02 * row_i2[k]     + C03 * row_i3[k];
                }
            }
            for (j = 0; j < i; j += 4) {
                const datatype b00 = a0[0] * row_i0[j]     + a0[1] * row_i1[j]     + a0[2] * row_i2[j]     + a0[3] * row_i3[j];
                const datatype b01 = a0[0] * row_i0[j + 1] + a0[1] * row_i1[j + 1] + a0[2] * row_i2[j + 1] + a0[3] * row_i3[j + 1];
                const datatype b02 = a0[0] * row_i0[j + 2] + a0[1] * row_i1[j + 2] + a0[2] * row_i2[j + 2] + a0[3] * row_i3[j + 2];
                const datatype b03 = a0[0] * row_i0[j + 3] + a0[1] * row_i1[j + 3] + a0[2] * row_i2[j + 3] + a0[3] * row_i3[j + 3];
                const datatype b10 = a1[0] * row_i0[j]     + a1[1] * row_i1[j]     + a1[2] * row_i2[j]     + a1[3] * row_i3[j];
                const datatype b11 = a1[0] * row_i0[j + 1] + a1[1] * row_i1[j + 1] + a1[2] * row_i2[j + 1] + a1[3] * row_i3[j + 1];
                const datatype b12 = a1[0] * row_i0[j + 2] + a1[1] * row_i1[j + 2] + a1[2] * row_i2[j + 2] + a1[3] * row_i3[j + 2];
                const datatype b13 = a1[0] * row_i0[j + 3] + a1[1] * row_i1[j + 3] + a1[2] * row_i2[j + 3] + a1[3] * row_i3[j + 3];
                const datatype b20 = a2[0] * row_i0[j]     + a2[1] * row_i1[j]     + a2[2] * row_i2[j]     + a2[3] * row_i3[j];
                const datatype b21 = a2[0] * row_i0[j + 1] + a2[1] * row_i1[j + 1] + a2[2] * row_i2[j + 1] + a2[3] * row_i3[j + 1];
                const datatype b22 = a2[0] * row_i0[j + 2] + a2[1] * row_i1[j + 2] + a2[2] * row_i2[j + 2] + a2[3] * row_i3[j + 2];
                const datatype b23 = a2[0] * row_i0[j + 3] + a2[1] * row_i1[j + 3] + a2[2] * row_i2[j + 3] + a2[3] * row_i3[j + 3];
                const datatype b30 = a3[0] * row_i0[j]     + a3[1] * row_i1[j]     + a3[2] * row_i2[j]     + a3[3] * row_i3[j];
                const datatype b31 = a3[0] * row_i0[j + 1] + a3[1] * row_i1[j + 1] + a3[2] * row_i2[j + 1] + a3[3] * row_i3[j + 1];
                const datatype b32 = a3[0] * row_i0[j + 2] + a3[1] * row_i1[j + 2] + a3[2] * row_i2[j + 2] + a3[3] * row_i3[j + 2];
                const datatype b33 = a3[0] * row_i0[j + 3] + a3[1] * row_i1[j + 3] + a3[2] * row_i2[j + 3] + a3[3] * row_i3[j + 3];
                row_i0[j]     = b00;   row_i0[j + 1] = b01;    row_i0[j + 2] = b02;    row_i0[j + 3] = b03;
                row_i1[j]     = b10;   row_i1[j + 1] = b11;    row_i1[j + 2] = b12;    row_i1[j + 3] = b13;
                row_i2[j]     = b20;   row_i2[j + 1] = b21;    row_i2[j + 2] = b22;    row_i2[j + 3] = b23;
                row_i3[j]     = b30;   row_i3[j + 1] = b31;    row_i3[j + 2] = b32;    row_i3[j + 3] = b33;
            }
            row_i0[j] = -a0[0];   row_i0[j + 1] = -a0[1];    row_i0[j + 2] = -a0[2];    row_i0[j + 3] = -a0[3]; // in case of non-negating inv = ai[j];
            row_i1[j] = -a1[0];   row_i1[j + 1] = -a1[1];    row_i1[j + 2] = -a1[2];    row_i1[j + 3] = -a1[3];
            row_i2[j] = -a2[0];   row_i2[j + 1] = -a2[1];    row_i2[j + 2] = -a2[2];    row_i2[j + 3] = -a2[3];
            row_i3[j] = -a3[0];   row_i3[j + 1] = -a3[1];    row_i3[j + 2] = -a3[2];    row_i3[j + 3] = -a3[3];
            for (j += 4; j < hrow; j += 4) {
                const datatype b00 = a0[0] * row_i0[j]     + a0[1] * row_i1[j]     + a0[2] * row_i2[j]     + a0[3] * row_i3[j];
                const datatype b01 = a0[0] * row_i0[j + 1] + a0[1] * row_i1[j + 1] + a0[2] * row_i2[j + 1] + a0[3] * row_i3[j + 1];
                const datatype b02 = a0[0] * row_i0[j + 2] + a0[1] * row_i1[j + 2] + a0[2] * row_i2[j + 2] + a0[3] * row_i3[j + 2];
                const datatype b03 = a0[0] * row_i0[j + 3] + a0[1] * row_i1[j + 3] + a0[2] * row_i2[j + 3] + a0[3] * row_i3[j + 3];
                const datatype b10 = a1[0] * row_i0[j]     + a1[1] * row_i1[j]     + a1[2] * row_i2[j]     + a1[3] * row_i3[j];
                const datatype b11 = a1[0] * row_i0[j + 1] + a1[1] * row_i1[j + 1] + a1[2] * row_i2[j + 1] + a1[3] * row_i3[j + 1];
                const datatype b12 = a1[0] * row_i0[j + 2] + a1[1] * row_i1[j + 2] + a1[2] * row_i2[j + 2] + a1[3] * row_i3[j + 2];
                const datatype b13 = a1[0] * row_i0[j + 3] + a1[1] * row_i1[j + 3] + a1[2] * row_i2[j + 3] + a1[3] * row_i3[j + 3];
                const datatype b20 = a2[0] * row_i0[j]     + a2[1] * row_i1[j]     + a2[2] * row_i2[j]     + a2[3] * row_i3[j];
                const datatype b21 = a2[0] * row_i0[j + 1] + a2[1] * row_i1[j + 1] + a2[2] * row_i2[j + 1] + a2[3] * row_i3[j + 1];
                const datatype b22 = a2[0] * row_i0[j + 2] + a2[1] * row_i1[j + 2] + a2[2] * row_i2[j + 2] + a2[3] * row_i3[j + 2];
                const datatype b23 = a2[0] * row_i0[j + 3] + a2[1] * row_i1[j + 3] + a2[2] * row_i2[j + 3] + a2[3] * row_i3[j + 3];
                const datatype b30 = a3[0] * row_i0[j]     + a3[1] * row_i1[j]     + a3[2] * row_i2[j]     + a3[3] * row_i3[j];
                const datatype b31 = a3[0] * row_i0[j + 1] + a3[1] * row_i1[j + 1] + a3[2] * row_i2[j + 1] + a3[3] * row_i3[j + 1];
                const datatype b32 = a3[0] * row_i0[j + 2] + a3[1] * row_i1[j + 2] + a3[2] * row_i2[j + 2] + a3[3] * row_i3[j + 2];
                const datatype b33 = a3[0] * row_i0[j + 3] + a3[1] * row_i1[j + 3] + a3[2] * row_i2[j + 3] + a3[3] * row_i3[j + 3];
                row_i0[j]     = b00;   row_i0[j + 1] = b01;    row_i0[j + 2] = b02;    row_i0[j + 3] = b03;
                row_i1[j]     = b10;   row_i1[j + 1] = b11;    row_i1[j + 2] = b12;    row_i1[j + 3] = b13;
                row_i2[j]     = b20;   row_i2[j + 1] = b21;    row_i2[j + 2] = b22;    row_i2[j + 3] = b23;
                row_i3[j]     = b30;   row_i3[j + 1] = b31;    row_i3[j + 2] = b32;    row_i3[j + 3] = b33;
            }
            for (j = hrow; j < nrow; j++) {
                const datatype b00 = a0[0] * row_i0[j]     + a0[1] * row_i1[j]     + a0[2] * row_i2[j]     + a0[3] * row_i3[j];
                const datatype b10 = a1[0] * row_i0[j]     + a1[1] * row_i1[j]     + a1[2] * row_i2[j]     + a1[3] * row_i3[j];
                const datatype b20 = a2[0] * row_i0[j]     + a2[1] * row_i1[j]     + a2[2] * row_i2[j]     + a2[3] * row_i3[j];
                const datatype b30 = a3[0] * row_i0[j]     + a3[1] * row_i1[j]     + a3[2] * row_i2[j]     + a3[3] * row_i3[j];
                row_i0[j]     = b00;
                row_i1[j]     = b10;
                row_i2[j]     = b20;
                row_i3[j]     = b30;
            }
        }
        for (unsigned i = hrow; i < nrow; i++) {
            vektor<datatype> & row_i = rows[i];
            datatype divisor = abs(row_i[i]) < 1e-20f ? 1e20f : datatype(1.0f) / row_i[i];
            unsigned j;
            for (j = 0; j < i; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = row_j[i] * divisor;
                unsigned k;
                for (k = 0; k < i; k++) row_j[k] -= C * row_i[k];
                row_j[k] = C; // in case of non-negating inv = -C;
                for (k++; k < row; k++) row_j[k] -= C * row_i[k];
            }
            for (j++; j < row; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = row_j[i] * divisor;
                unsigned k;
                for (k = 0; k < i; k++) row_j[k] -= C * row_i[k];
                row_j[k] = C; // in case of non-negating inv = -C;
                for (k++; k < row; k++) row_j[k] -= C * row_i[k];
            }
            for (j = 0; j < i; j++) row_i[j] *= divisor;
            row_i[j] = -divisor; // in case of non-negating inv = divisor;
            for (j++; j < row; j++) row_i[j] *= divisor;
        }
    }

    //****************************************************************
    void math_inv_p(bool neg) { // cannot be noexcept!
    // with pivoting
    // Not laying-proof
    //****************************************************************
        if (col == 0 || row == 0)
            return;
        is_equal_error(row, col, "matrix::math_inv_p");
        is_true_error(is_symm, "matrix::math_ninv_p", "symmetrical matrix not allowed");

        const unsigned S2 = row + row;
        unsigned *x = new unsigned[S2], *y = x + row;
        for (unsigned i = 0; i < S2; i++) x[i] = ~0;

        for (unsigned i = 0; i < row; i++) {

            // choosing pivot

            vektor<datatype> & row_i = rows[i];
            double diff = 0.0;
            unsigned V = ~0;
            unsigned j;
            for (j = 0; j < row; j++) if (x[j] == ~0) {
                double temp = abs(row_i[j]);
                if (temp>diff) { diff = temp; V = j; }//v-edik oszlopot választjuk
            }
            if ((V == ~0) || (diff == 0))
                throw hmgExcept("matrix::math_inv_p", "singular matrix");
            x[V] = i;
            y[i] = V;

            // element replace

            datatype A = datatype(1.0) / row_i.unsafe(V);

            for (j = 0; j < i; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = -row_j.unsafe(V) * A;
                unsigned k;
                for (k = 0; k < V; k++)row_j[k] += C*row_i[k];
                row_j[k] = C;
                for (k++; k < row; k++)row_j[k] += C*row_i[k];
            }
            for (j++; j < row; j++) {
                vektor<datatype> & row_j = rows[j];
                datatype C = -row_j.unsafe(V) * A;
                unsigned k;
                for (k = 0; k < V; k++)row_j[k] += C*row_i[k];
                row_j[k] = C;
                for (k++; k < row; k++)row_j[k] += C*row_i[k];
            }
            for (j = 0;j<V;j++)row_i[j] *= A;
            row_i[j] = A;
            for (j++; j < row; j++)row_i[j] *= A;
        }

        // Order rows and columns

        datatype * N = &t[0];
        unsigned i;
        for (i = 0; i < row; i++) {
            unsigned j;
            for (j = i; y[j] != i; j++)
                ;
            if (i != j) {
                vektor<datatype> & row_i = rows[i];
                vektor<datatype> & row_j = rows[j];
                for (unsigned k = 0; k < row; k++) { datatype temp = row_i[k]; row_i[k] = row_j[k]; row_j[k] = temp; }
                y[j] = y[i];
            }
        }
        for (i = 0;i < row; i++) {
            unsigned j;
            for (j = i; x[j] != i; j++)
                ;
            if (i != j) {
                for (unsigned k = 0; k < row; k++) { 
                    vektor<datatype> & sor_k = rows[k]; 
                    datatype temp  = sor_k[i];
                    sor_k[i] = sor_k[j];
                    sor_k[j] = temp; 
                }
                x[j] = x[i];
            }
        }

        delete[] x;
        if (neg) math_neg_unsafe();
    }

    //***********************************************************************
    void zero_unsafe() noexcept {
    // Not laying-proof
    //***********************************************************************
        t.zero();
    }

    //***********************************************************************
    void identity() noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(is_symm, "matrix::identity", "symmetrical matrix not allowed");
        t.zero();
        for (unsigned i = 0; i < row; i++)
            rows[i][i] = 1.0;
    }

    //***********************************************************************
    void math_1_ninv_mul(matrix & yb, const matrix & xa) noexcept {
    // yb is 1x1, this is inverted and puts zb*xa inside itself
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        const datatype nzb = datatype(-1) / yb.rows[0][0];
        yb.rows[0][0] = nzb;
        for (unsigned i = 0; i < col; i++)
            rows[0][i] = nzb*xa.rows[0][i];
    }

    //***********************************************************************
    void math_1_ninv_mulT(matrix & yb, const matrix & xat) noexcept {
    // yb is 1x1, this is inverted and puts zb*xa inside itself
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        const datatype nzb = datatype(-1) / yb.rows[0][0];
        yb.rows[0][0] = nzb;
        for (unsigned i = 0; i < col; i++)
            rows[0][i] = nzb*xat.rows[i][0];
    }

    //***********************************************************************
    void math_2_ninv_mul(matrix & yb, const matrix & xa) noexcept(!hmgVErrorCheck) {
    // yb is 2x2, this is inverted and puts zb*xa inside itself
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(yb.is_symm, "matrix::math_2_ninv_mul", "symmetrical matrix not allowed");
        datatype * in0 = &yb.rows[0][0];
        datatype * in1 = &yb.rows[1][0];
        const datatype p0 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        const datatype p2 = -in1[0] * p0;
        const datatype p1 = in0[1] * p0;
        const datatype p3 = in1[1] + p2 * in0[1];
        const datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
        const datatype C2 = -p1 * divisor2;
        const datatype nzb00 = in0[0] = -(p0 + C2 * p2);
        const datatype nzb01 = in0[1] = -C2;
        const datatype nzb10 = in1[0] = -p2 * divisor2;
        const datatype nzb11 = in1[1] = -divisor2;

        vektor<datatype> & nzbxa0 = rows[0];
        vektor<datatype> & nzbxa1 = rows[1];
        const vektor<datatype> & xa0 = xa.rows[0];
        const vektor<datatype> & xa1 = xa.rows[1];
        for (unsigned i = 0; i < col; i++) {
            nzbxa0[i] = nzb00 * xa0[i] + nzb01 * xa1[i];
            nzbxa1[i] = nzb10 * xa0[i] + nzb11 * xa1[i];
        }
    }

    //***********************************************************************
    void math_2_ninv_mulT(matrix & yb, const matrix & xat) noexcept(!hmgVErrorCheck) {
    // yb is 2x2, this is inverted and puts zb*xa inside itself
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(yb.is_symm, "matrix::math_2_ninv_mul", "symmetrical matrix not allowed");
        datatype * in0 = &yb.rows[0][0];
        datatype * in1 = &yb.rows[1][0];
        const datatype p0 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        const datatype p2 = -in1[0] * p0;
        const datatype p1 = in0[1] * p0;
        const datatype p3 = in1[1] + p2 * in0[1];
        const datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
        const datatype C2 = -p1 * divisor2;
        const datatype nzb00 = in0[0] = -(p0 + C2 * p2);
        const datatype nzb01 = in0[1] = -C2;
        const datatype nzb10 = in1[0] = -p2 * divisor2;
        const datatype nzb11 = in1[1] = -divisor2;

        vektor<datatype> & nzbxa0 = rows[0];
        vektor<datatype> & nzbxa1 = rows[1];
        for (unsigned i = 0; i < col; i++) {
            nzbxa0[i] = nzb00 * xat[i][0] + nzb01 * xat[i][1];
            nzbxa1[i] = nzb10 * xat[i][0] + nzb11 * xat[i][1];
        }
    }

    //***********************************************************************
    void math_2_ninv_mul_symm(matrix & yb, const matrix & xa) noexcept(!hmgVErrorCheck) {
    // yb is 2x2, stored whole symmetrical, this is inverted and puts zb*xa inside itself
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(yb.is_symm, "matrix::math_2_ninv_mul", "symmetrical matrix not allowed");
        datatype * in0 = &yb.rows[0][0];
        datatype * in1 = &yb.rows[1][0];
        const datatype p0 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        const datatype p2 = -in0[1] * p0;
        const datatype p3 = in1[1] + p2 * in0[1];
        const datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
        const datatype C2 = -p2 * divisor2;
        const datatype nzb00 = in0[0] = C2 * p2 - p0;
        const datatype nzb01 = in0[1] = in1[0] = C2;
        const datatype nzb11 = in1[1] = -divisor2;

        vektor<datatype> & nzbxa0 = rows[0];
        vektor<datatype> & nzbxa1 = rows[1];
        const vektor<datatype> & xa0 = xa.rows[0];
        const vektor<datatype> & xa1 = xa.rows[1];
        for (unsigned i = 0; i < col; i++) {
            nzbxa0[i] = nzb00 * xa0[i] + nzb01 * xa1[i];
            nzbxa1[i] = nzb01 * xa0[i] + nzb11 * xa1[i];
        }
    }

    //***********************************************************************
    void math_2_ninv_mul_symmT(matrix & yb, const matrix & xat) noexcept(!hmgVErrorCheck) {
    // yb is 2x2, stored whole symmetrical, this is inverted and puts zb*xa inside itself
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(yb.is_symm, "matrix::math_2_ninv_mul", "symmetrical matrix not allowed");
        datatype * in0 = &yb.rows[0][0];
        datatype * in1 = &yb.rows[1][0];
        const datatype p0 = abs(in0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / in0[0];
        const datatype p2 = -in0[1] * p0;
        const datatype p3 = in1[1] + p2 * in0[1];
        const datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
        const datatype C2 = -p2 * divisor2;
        const datatype nzb00 = in0[0] = C2 * p2 - p0;
        const datatype nzb01 = in0[1] = in1[0] = C2;
        const datatype nzb11 = in1[1] = -divisor2;

        vektor<datatype> & nzbxa0 = rows[0];
        vektor<datatype> & nzbxa1 = rows[1];
        //const vektor<datatype> & xa0 = xa.rows[0];
        //const vektor<datatype> & xa1 = xa.rows[1];
        for (unsigned i = 0; i < col; i++) {
            nzbxa0[i] = nzb00 * xat[i][0] + nzb01 * xat[i][1];
            nzbxa1[i] = nzb01 * xat[i][0] + nzb11 * xat[i][1];
        }
    }

    //***********************************************************************
    void math_1_add_mul(const matrix & ya, const matrix & xb, const matrix & nzbxa) noexcept(!hmgVErrorCheck) {
    // xb is 1 column, nzbxa is 1 row
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(ya.is_symm || is_symm, "matrix::math_1_add_mul", "symmetrical matrix not allowed");
        for (unsigned i = 0; i < row; i++) {
            const datatype xbe = xb.rows[i][0];
            for (unsigned j = 0; j < col; j++)
                rows[i][j] = ya.rows[i][j] + xbe * nzbxa.rows[0][j];
        }
    }

    //***********************************************************************
    void math_1_add_mul_symm(const matrix & ya, const matrix & xb, const matrix & nzbxa) noexcept(!hmgVErrorCheck) {
    // xb is 1 column, nzbxa is 1 row
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(!ya.is_symm || !is_symm, "matrix::math_1_add_mul_symm", "nonsymmetrical matrix not allowed");
        for (unsigned i = 0; i < row; i++) {
            const datatype xbe = xb.rows[i][0];
            for (unsigned j = i; j < col; j++)
                rows[i][j] = ya.rows[i][j] + xbe * nzbxa.rows[0][j];
        }
    }

    //***********************************************************************
    void math_2_add_mul(const matrix & ya, const matrix & xb, const matrix & nzbxa) noexcept(!hmgVErrorCheck) {
    // xb is 2 column, nzbxa is 2 row
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(ya.is_symm || is_symm, "matrix::math_2_add_mul", "symmetrical matrix not allowed");
        const vektor<datatype> & nzbxa0 = nzbxa.rows[0];
        const vektor<datatype> & nzbxa1 = nzbxa.rows[1];
        for (unsigned i = 0; i < row; i++) {
            const datatype xb0 = xb.rows[i][0];
            const datatype xb1 = xb.rows[i][1];
            for (unsigned j = 0; j < col; j++)
                rows[i][j] = ya.rows[i][j] + xb0 * nzbxa0[j] + xb1 * nzbxa1[j];
        }
    }

    //***********************************************************************
    void math_2_add_mul_symm(const matrix & ya, const matrix & xb, const matrix & nzbxa) noexcept(!hmgVErrorCheck) {
    // xb is 2 column, nzbxa is 2 row
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(!ya.is_symm || !is_symm, "matrix::math_2_add_mul_symm", "symmetrical matrix not allowed");
        const vektor<datatype> & nzbxa0 = nzbxa.rows[0];
        const vektor<datatype> & nzbxa1 = nzbxa.rows[1];
        for (unsigned i = 0; i < row; i++) {
            const datatype xb0 = xb.rows[i][0];
            const datatype xb1 = xb.rows[i][1];
            for (unsigned j = i; j < col; j++)
                rows[i][j] = ya.rows[i][j] + xb0 * nzbxa0[j] + xb1 * nzbxa1[j];
        }
    }

    //***********************************************************************
    friend inline void math_1x1_mul(vektor<datatype> & nzbjb, const matrix & nzb, const vektor<datatype> & jb) noexcept {
    // everithing is 1 sized
    //***********************************************************************
        nzbjb[0] = nzb.rows[0][0] * jb[0];
    }

    //***********************************************************************
    friend inline void math_2x2_mul(vektor<datatype> & nzbjb, const matrix & nzb, const vektor<datatype> & jb) noexcept(!hmgVErrorCheck) {
    // nzb is 2x2
    //***********************************************************************
        is_true_error(nzb.is_symm, "math_2x2_mul", "symmetrical matrix not allowed");
        nzbjb[0] = nzb.rows[0][0] * jb[0] + nzb.rows[0][1] * jb[1];
        nzbjb[1] = nzb.rows[1][0] * jb[0] + nzb.rows[1][1] * jb[1];
    }

    //***********************************************************************
    friend inline void math_1_add_mul_jred(vektor<datatype> & jred, const vektor<datatype> & ja, const matrix & xb, const vektor<datatype> & nzbjb) noexcept {
    // size of nzbjb is 1
    //***********************************************************************
        const datatype a = nzbjb[0];
        for (unsigned i = 0; i < jred.size(); i++)
            jred[i] = ja[i] + xb.rows[i][0] * a;
    }

    //***********************************************************************
    friend inline void math_2_add_mul_jred(vektor<datatype> & jred, const vektor<datatype> & ja, const matrix & xb, const vektor<datatype> & nzbjb) noexcept {
    // size of nzbjb is 2
    //***********************************************************************
        const datatype a0 = nzbjb[0];
        const datatype a1 = nzbjb[1];
        for (unsigned i = 0; i < jred.size(); i++)
            jred[i] = ja[i] + xb.rows[i][0] * a0 + xb.rows[i][1] * a1;
    }

    //***********************************************************************
    friend inline void math_1_add_mul_ub(vektor<datatype> & ub, const vektor<datatype> & nzbjb, const matrix & nzbxa, const vektor<datatype> & UA) noexcept {
    // size of ub is 1
    //***********************************************************************
        ub[0] = nzbjb[0] + math_mul(nzbxa.rows[0], UA);
    }

    //***********************************************************************
    friend inline void math_2_add_mul_ub(vektor<datatype> & ub, const vektor<datatype> & nzbjb, const matrix & nzbxa, const vektor<datatype> & UA) noexcept {
    // size of ub is 2
    //***********************************************************************
        ub[0] = nzbjb[0] + math_mul(nzbxa.rows[0], UA);
        ub[1] = nzbjb[1] + math_mul(nzbxa.rows[1], UA);
    }
    
    //***********************************************************************
    void math_symm_ninv_of_nonsymm() noexcept(!hmgVErrorCheck) {
    // The symmetric matrix is stored in a non-symmetric form, it can also be layed
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(is_symm, "math_symm_ninv_of_nonsymm_rafektetett", "only nonsymmetrically stored symmetrical matrix allowed");
#define NEWSUBSTITUTOR 32

        if (row == 1) {
            rows[0][0] = datatype(-1) / rows[0][0];
            return;
        }
        if (row == 2) {
            datatype * const ps0 = &rows[0][0];
            datatype * const ps1 = &rows[1][0];
            datatype p0 = abs(ps0[0]) < 1e-20f ? 1e20f : datatype(1.0f) / ps0[0];
            datatype p2 = -ps0[1] * p0;
            datatype p3 = ps1[1] + p2 * ps0[1];
            datatype divisor2 = abs(p3) < 1e-20f ? 1e20f : datatype(1.0f) / p3;
            datatype C2 = -p2 * divisor2;
            ps0[0] = C2 * p2 - p0;
            ps1[0] = ps0[1] = C2;
            ps1[1] = -divisor2;
            return;
        }

        datatype dum[2 * NEWSUBSTITUTOR], *b0 = row > NEWSUBSTITUTOR ? new datatype[2 * row] : dum, *bs = b0 + row;
        unsigned i, j, k;

        for (i = 0; i < row; i++) {
            vektor<datatype> & row_i = rows[i];
            datatype & pivot = row_i[i];
            datatype divisor = abs(pivot) < 1e-20f ? 1e20f : datatype(1.0f) / pivot;
            for (k = 0; k < i; ++k) { datatype & v = rows[k][i]; b0[k] = v;	    bs[k] = v *= divisor; }
                                                                 b0[k] = pivot;	bs[k] = pivot = -divisor;
            for (k++; k < row; k++) { datatype & v = row_i[k];   b0[k] = v;	    bs[k] = v *= divisor; }

            for (j = 0; j < i; j++) {//from j=0 to i-1
                vektor<datatype> & row_j = rows[j];
                const datatype x = b0[j]; // jth element of row i
                const datatype x2 = row_j[i]; // ith element of row j
                for (k = j; k < row; k++) row_j[k] -= x*bs[k];
                row_j[i] = x2;
            }
            for (j++; j < row; j++) {//from j=i+1 to S-1
                vektor<datatype> & row_j = rows[j];
                const datatype x = b0[j];
                for (k = j; k < row; k++) row_j[k] -= x*bs[k];//k!=i is always true because the loop starts from j=i+1
            }
        }
        if (row > NEWSUBSTITUTOR)delete[] b0;
        symmetrize_from_upper();
    }

    //***********************************************************************
    void symmetrize_from_upper() noexcept(!hmgVErrorCheck) {
    // Copy the upper triangle of a non-symmetric matrix to the lower one
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(is_symm, "symmetrize_from_upper", "only nonsymmetrically stored symmetrical matrix allowed");
        is_equal_error(row, col, "matrix::symmetrize_from_upper");
        for (unsigned i = 1; i < col; i++)
            for (unsigned j = 0; j < i; j++) {
                rows[i][j] = rows[j][i];
            }
    }

    //***********************************************************************
    void math_add_mul_t_symm(const matrix & ya, const matrix & xb, const matrix & nzbxat) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(!is_symm || !ya.is_symm, "matrix::math_add_mul_t_symm", "symmetrical matrix required");
        is_equal_error(ya.col, col, "math_add_mul_t_symm col");
        is_equal_error(xb.row, row, "math_add_mul_t_symm row row");
        is_equal_error(nzbxat.row, col, "math_add_mul_t_symm row col");
        is_equal_error(xb.col, nzbxat.col, "math_add_mul_t_symm col col");

        const unsigned ni = row, nj = col, nk = xb.col;
        const unsigned di = row % 4, dj = col % 4, dk = xb.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        for (unsigned i = 0; i < hi; i += 4) {
            const vektor<datatype> & xb_row_i0 = xb.rows[i + 0];
            const vektor<datatype> & xb_row_i1 = xb.rows[i + 1];
            const vektor<datatype> & xb_row_i2 = xb.rows[i + 2];
            const vektor<datatype> & xb_row_i3 = xb.rows[i + 3];
            const vektor<datatype> & nzbxat_row_i0 = nzbxat.rows[i + 0];
            const vektor<datatype> & nzbxat_row_i1 = nzbxat.rows[i + 1];
            const vektor<datatype> & nzbxat_row_i2 = nzbxat.rows[i + 2];
            const vektor<datatype> & nzbxat_row_i3 = nzbxat.rows[i + 3];
            datatype triangle[10] = { datatype() };
            for (unsigned k = 0; k < hk; k += 4) {
                triangle[0] += xb_row_i0[k + 0] * nzbxat_row_i0[k + 0] 
                              + xb_row_i0[k + 1] * nzbxat_row_i0[k + 1] 
                              + xb_row_i0[k + 2] * nzbxat_row_i0[k + 2] 
                              + xb_row_i0[k + 3] * nzbxat_row_i0[k + 3];
                triangle[1] += xb_row_i0[k + 0] * nzbxat_row_i1[k + 0] 
                              + xb_row_i0[k + 1] * nzbxat_row_i1[k + 1] 
                              + xb_row_i0[k + 2] * nzbxat_row_i1[k + 2] 
                              + xb_row_i0[k + 3] * nzbxat_row_i1[k + 3];
                triangle[2] += xb_row_i0[k + 0] * nzbxat_row_i2[k + 0] 
                              + xb_row_i0[k + 1] * nzbxat_row_i2[k + 1] 
                              + xb_row_i0[k + 2] * nzbxat_row_i2[k + 2] 
                              + xb_row_i0[k + 3] * nzbxat_row_i2[k + 3];
                triangle[3] += xb_row_i0[k + 0] * nzbxat_row_i3[k + 0] 
                              + xb_row_i0[k + 1] * nzbxat_row_i3[k + 1] 
                              + xb_row_i0[k + 2] * nzbxat_row_i3[k + 2] 
                              + xb_row_i0[k + 3] * nzbxat_row_i3[k + 3];

                triangle[4] += xb_row_i1[k + 0] * nzbxat_row_i1[k + 0] 
                              + xb_row_i1[k + 1] * nzbxat_row_i1[k + 1] 
                              + xb_row_i1[k + 2] * nzbxat_row_i1[k + 2] 
                              + xb_row_i1[k + 3] * nzbxat_row_i1[k + 3];
                triangle[5] += xb_row_i1[k + 0] * nzbxat_row_i2[k + 0] 
                              + xb_row_i1[k + 1] * nzbxat_row_i2[k + 1] 
                              + xb_row_i1[k + 2] * nzbxat_row_i2[k + 2] 
                              + xb_row_i1[k + 3] * nzbxat_row_i2[k + 3];
                triangle[6] += xb_row_i1[k + 0] * nzbxat_row_i3[k + 0] 
                              + xb_row_i1[k + 1] * nzbxat_row_i3[k + 1] 
                              + xb_row_i1[k + 2] * nzbxat_row_i3[k + 2] 
                              + xb_row_i1[k + 3] * nzbxat_row_i3[k + 3];

                triangle[7] += xb_row_i2[k + 0] * nzbxat_row_i2[k + 0] 
                              + xb_row_i2[k + 1] * nzbxat_row_i2[k + 1] 
                              + xb_row_i2[k + 2] * nzbxat_row_i2[k + 2] 
                              + xb_row_i2[k + 3] * nzbxat_row_i2[k + 3];
                triangle[8] += xb_row_i2[k + 0] * nzbxat_row_i3[k + 0] 
                              + xb_row_i2[k + 1] * nzbxat_row_i3[k + 1] 
                              + xb_row_i2[k + 2] * nzbxat_row_i3[k + 2] 
                              + xb_row_i2[k + 3] * nzbxat_row_i3[k + 3];

                triangle[9] += xb_row_i3[k + 0] * nzbxat_row_i3[k + 0] 
                              + xb_row_i3[k + 1] * nzbxat_row_i3[k + 1] 
                              + xb_row_i3[k + 2] * nzbxat_row_i3[k + 2] 
                              + xb_row_i3[k + 3] * nzbxat_row_i3[k + 3];
            }
            for (unsigned k = hk; k < nk; k++) {
                triangle[0] += xb_row_i0[k] * nzbxat_row_i0[k];
                triangle[1] += xb_row_i0[k] * nzbxat_row_i1[k];
                triangle[2] += xb_row_i0[k] * nzbxat_row_i2[k];
                triangle[3] += xb_row_i0[k] * nzbxat_row_i3[k];

                triangle[4] += xb_row_i1[k] * nzbxat_row_i1[k];
                triangle[5] += xb_row_i1[k] * nzbxat_row_i2[k];
                triangle[6] += xb_row_i1[k] * nzbxat_row_i3[k];

                triangle[7] += xb_row_i2[k] * nzbxat_row_i2[k];
                triangle[8] += xb_row_i2[k] * nzbxat_row_i3[k];

                triangle[9] += xb_row_i3[k] * nzbxat_row_i3[k];

            }

            vektor<datatype> & row_i0 = rows[i + 0];
            vektor<datatype> & row_i1 = rows[i + 1];
            vektor<datatype> & row_i2 = rows[i + 2];
            vektor<datatype> & row_i3 = rows[i + 3];
            const vektor<datatype> & ya_row_i0 = ya.rows[i + 0];
            const vektor<datatype> & ya_row_i1 = ya.rows[i + 1];
            const vektor<datatype> & ya_row_i2 = ya.rows[i + 2];
            const vektor<datatype> & ya_row_i3 = ya.rows[i + 3];

            row_i0[i + 0] = triangle[0] + ya_row_i0[i + 0];
            row_i0[i + 1] = triangle[1] + ya_row_i0[i + 1];
            row_i0[i + 2] = triangle[2] + ya_row_i0[i + 2];
            row_i0[i + 3] = triangle[3] + ya_row_i0[i + 3];
            
            row_i1[i + 1] = triangle[4] + ya_row_i1[i + 1];
            row_i1[i + 2] = triangle[5] + ya_row_i1[i + 2];
            row_i1[i + 3] = triangle[6] + ya_row_i1[i + 3];

            row_i2[i + 2] = triangle[7] + ya_row_i2[i + 2];
            row_i2[i + 3] = triangle[8] + ya_row_i2[i + 3];

            row_i3[i + 3] = triangle[9] + ya_row_i3[i + 3];

            for (unsigned j = i + 4; j < hj; j += 4) {
                const vektor<datatype> & nzbxat_row_j0 = nzbxat.rows[j + 0];
                const vektor<datatype> & nzbxat_row_j1 = nzbxat.rows[j + 1];
                const vektor<datatype> & nzbxat_row_j2 = nzbxat.rows[j + 2];
                const vektor<datatype> & nzbxat_row_j3 = nzbxat.rows[j + 3];
                datatype square[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4) {
                    square[0] += xb_row_i0[k + 0] * nzbxat_row_j0[k + 0] 
                                + xb_row_i0[k + 1] * nzbxat_row_j0[k + 1] 
                                + xb_row_i0[k + 2] * nzbxat_row_j0[k + 2] 
                                + xb_row_i0[k + 3] * nzbxat_row_j0[k + 3];
                    square[1] += xb_row_i0[k + 0] * nzbxat_row_j1[k + 0] 
                                + xb_row_i0[k + 1] * nzbxat_row_j1[k + 1] 
                                + xb_row_i0[k + 2] * nzbxat_row_j1[k + 2] 
                                + xb_row_i0[k + 3] * nzbxat_row_j1[k + 3];
                    square[2] += xb_row_i0[k + 0] * nzbxat_row_j2[k + 0] 
                                + xb_row_i0[k + 1] * nzbxat_row_j2[k + 1] 
                                + xb_row_i0[k + 2] * nzbxat_row_j2[k + 2] 
                                + xb_row_i0[k + 3] * nzbxat_row_j2[k + 3];
                    square[3] += xb_row_i0[k + 0] * nzbxat_row_j3[k + 0] 
                                + xb_row_i0[k + 1] * nzbxat_row_j3[k + 1] 
                                + xb_row_i0[k + 2] * nzbxat_row_j3[k + 2] 
                                + xb_row_i0[k + 3] * nzbxat_row_j3[k + 3];

                    square[4] += xb_row_i1[k + 0] * nzbxat_row_j0[k + 0] 
                                + xb_row_i1[k + 1] * nzbxat_row_j0[k + 1]
                                + xb_row_i1[k + 2] * nzbxat_row_j0[k + 2]
                                + xb_row_i1[k + 3] * nzbxat_row_j0[k + 3];
                    square[5] += xb_row_i1[k + 0] * nzbxat_row_j1[k + 0]
                                + xb_row_i1[k + 1] * nzbxat_row_j1[k + 1]
                                + xb_row_i1[k + 2] * nzbxat_row_j1[k + 2]
                                + xb_row_i1[k + 3] * nzbxat_row_j1[k + 3];
                    square[6] += xb_row_i1[k + 0] * nzbxat_row_j2[k + 0]
                                + xb_row_i1[k + 1] * nzbxat_row_j2[k + 1]
                                + xb_row_i1[k + 2] * nzbxat_row_j2[k + 2]
                                + xb_row_i1[k + 3] * nzbxat_row_j2[k + 3];
                    square[7] += xb_row_i1[k + 0] * nzbxat_row_j3[k + 0]
                                + xb_row_i1[k + 1] * nzbxat_row_j3[k + 1]
                                + xb_row_i1[k + 2] * nzbxat_row_j3[k + 2]
                                + xb_row_i1[k + 3] * nzbxat_row_j3[k + 3];

                    square[8] += xb_row_i2[k + 0] * nzbxat_row_j0[k + 0] 
                                + xb_row_i2[k + 1] * nzbxat_row_j0[k + 1]
                                + xb_row_i2[k + 2] * nzbxat_row_j0[k + 2]
                                + xb_row_i2[k + 3] * nzbxat_row_j0[k + 3];
                    square[9] += xb_row_i2[k + 0] * nzbxat_row_j1[k + 0]
                                + xb_row_i2[k + 1] * nzbxat_row_j1[k + 1]
                                + xb_row_i2[k + 2] * nzbxat_row_j1[k + 2]
                                + xb_row_i2[k + 3] * nzbxat_row_j1[k + 3];
                    square[10]+= xb_row_i2[k + 0] * nzbxat_row_j2[k + 0]
                                + xb_row_i2[k + 1] * nzbxat_row_j2[k + 1]
                                + xb_row_i2[k + 2] * nzbxat_row_j2[k + 2]
                                + xb_row_i2[k + 3] * nzbxat_row_j2[k + 3];
                    square[11]+= xb_row_i2[k + 0] * nzbxat_row_j3[k + 0]
                                + xb_row_i2[k + 1] * nzbxat_row_j3[k + 1]
                                + xb_row_i2[k + 2] * nzbxat_row_j3[k + 2]
                                + xb_row_i2[k + 3] * nzbxat_row_j3[k + 3];

                    square[12]+= xb_row_i3[k + 0] * nzbxat_row_j0[k + 0] 
                                + xb_row_i3[k + 1] * nzbxat_row_j0[k + 1]
                                + xb_row_i3[k + 2] * nzbxat_row_j0[k + 2]
                                + xb_row_i3[k + 3] * nzbxat_row_j0[k + 3];
                    square[13]+= xb_row_i3[k + 0] * nzbxat_row_j1[k + 0]
                                + xb_row_i3[k + 1] * nzbxat_row_j1[k + 1]
                                + xb_row_i3[k + 2] * nzbxat_row_j1[k + 2]
                                + xb_row_i3[k + 3] * nzbxat_row_j1[k + 3];
                    square[14]+= xb_row_i3[k + 0] * nzbxat_row_j2[k + 0]
                                + xb_row_i3[k + 1] * nzbxat_row_j2[k + 1]
                                + xb_row_i3[k + 2] * nzbxat_row_j2[k + 2]
                                + xb_row_i3[k + 3] * nzbxat_row_j2[k + 3];
                    square[15]+= xb_row_i3[k + 0] * nzbxat_row_j3[k + 0]
                                + xb_row_i3[k + 1] * nzbxat_row_j3[k + 1]
                                + xb_row_i3[k + 2] * nzbxat_row_j3[k + 2]
                                + xb_row_i3[k + 3] * nzbxat_row_j3[k + 3];
                }
                for (unsigned k = hk; k < nk; k++) {
                    square[0]  += xb_row_i0[k] * nzbxat_row_j0[k];
                    square[1]  += xb_row_i0[k] * nzbxat_row_j1[k];
                    square[2]  += xb_row_i0[k] * nzbxat_row_j2[k];
                    square[3]  += xb_row_i0[k] * nzbxat_row_j3[k];

                    square[4]  += xb_row_i1[k] * nzbxat_row_j0[k];
                    square[5]  += xb_row_i1[k] * nzbxat_row_j1[k];
                    square[6]  += xb_row_i1[k] * nzbxat_row_j2[k];
                    square[7]  += xb_row_i1[k] * nzbxat_row_j3[k];

                    square[8]  += xb_row_i2[k] * nzbxat_row_j0[k];
                    square[9]  += xb_row_i2[k] * nzbxat_row_j1[k];
                    square[10] += xb_row_i2[k] * nzbxat_row_j2[k];
                    square[11] += xb_row_i2[k] * nzbxat_row_j3[k];

                    square[12] += xb_row_i3[k] * nzbxat_row_j0[k];
                    square[13] += xb_row_i3[k] * nzbxat_row_j1[k];
                    square[14] += xb_row_i3[k] * nzbxat_row_j2[k];
                    square[15] += xb_row_i3[k] * nzbxat_row_j3[k];
                }

                row_i0[j + 0] = square[0]  + ya_row_i0[j + 0];
                row_i0[j + 1] = square[1]  + ya_row_i0[j + 1];
                row_i0[j + 2] = square[2]  + ya_row_i0[j + 2];
                row_i0[j + 3] = square[3]  + ya_row_i0[j + 3];

                row_i1[j + 0] = square[4]  + ya_row_i1[j + 0];
                row_i1[j + 1] = square[5]  + ya_row_i1[j + 1];
                row_i1[j + 2] = square[6]  + ya_row_i1[j + 2];
                row_i1[j + 3] = square[7]  + ya_row_i1[j + 3];

                row_i2[j + 0] = square[8]  + ya_row_i2[j + 0];
                row_i2[j + 1] = square[9]  + ya_row_i2[j + 1];
                row_i2[j + 2] = square[10] + ya_row_i2[j + 2];
                row_i2[j + 3] = square[11] + ya_row_i2[j + 3];

                row_i3[j + 0] = square[12] + ya_row_i3[j + 0];
                row_i3[j + 1] = square[13] + ya_row_i3[j + 1];
                row_i3[j + 2] = square[14] + ya_row_i3[j + 2];
                row_i3[j + 3] = square[15] + ya_row_i3[j + 3];
            }
            for (unsigned j = hj; j < nj; j++) {
                const vektor<datatype> & nzbxat_row_j0 = nzbxat.rows[j];
                datatype quad[4] = { datatype() };
                for (unsigned k = 0; k < nk; k++) {
                    quad[0] += xb_row_i0[k] * nzbxat_row_j0[k];
                    quad[1] += xb_row_i1[k] * nzbxat_row_j0[k];
                    quad[2] += xb_row_i2[k] * nzbxat_row_j0[k];
                    quad[3] += xb_row_i3[k] * nzbxat_row_j0[k];
                }
                row_i0[j] = quad[0] + ya_row_i0[j];
                row_i1[j] = quad[1] + ya_row_i1[j];
                row_i2[j] = quad[2] + ya_row_i2[j];
                row_i3[j] = quad[3] + ya_row_i3[j];
            }
	    }
        for (unsigned i = hi; i < ni; i++) {
            const vektor<datatype> & xb_row_i = xb.rows[i];
            for (unsigned j = i; j < nj; j++) {
                const vektor<datatype> & nzbxat_sor_j = nzbxat.rows[j];
                datatype sum = datatype();
                for (unsigned k = 0; k < nk; k++) {
                    sum += xb_row_i[k] * nzbxat_sor_j[k];
                }
                rows[i][j] = sum + ya.rows[i][j];
            }
        }
    }

    //***********************************************************************
    void math_sub_mul_t_symm_in_nonsymm(const matrix & c, const matrix & a, const matrix & b, bool is_symmetrize_needed) noexcept(!hmgVErrorCheck) {
    //***********************************************************************
        if (col == 0 || row == 0)
            return;
        is_true_error(is_symm || c.is_symm, "matrix::math_sub_mul_t_symm_in_nonsymm", "nonsymmetrical matrix required");
        is_equal_error(c.col, col, "math_sub_mul_t_symm_in_nonsymm col");
        is_equal_error(a.row, row, "math_sub_mul_t_symm_in_nonsymm row row");
        is_equal_error(b.row, col, "math_sub_mul_t_symm_in_nonsymm row col");
        is_equal_error(a.col, b.col, "math_sub_mul_t_symm_in_nonsymm col col");

        const unsigned ni = row, nj = col, nk = a.col;
        const unsigned di = row % 4, dj = col % 4, dk = a.col % 4;
        const unsigned hi = ni - di, hj = nj - dj, hk = nk - dk;
        for (unsigned i = 0; i < hi; i += 4) {
            const vektor<datatype> & xb_row_i0 = a.rows[i + 0];
            const vektor<datatype> & xb_row_i1 = a.rows[i + 1];
            const vektor<datatype> & xb_row_i2 = a.rows[i + 2];
            const vektor<datatype> & xb_row_i3 = a.rows[i + 3];
            const vektor<datatype> & nzbxat_row_i0 = b.rows[i + 0];
            const vektor<datatype> & nzbxat_row_i1 = b.rows[i + 1];
            const vektor<datatype> & nzbxat_row_i2 = b.rows[i + 2];
            const vektor<datatype> & nzbxat_row_i3 = b.rows[i + 3];
            datatype triangle[10] = { datatype() };
            for (unsigned k = 0; k < hk; k += 4) {
                triangle[0] += xb_row_i0[k + 0] * nzbxat_row_i0[k + 0] 
                             + xb_row_i0[k + 1] * nzbxat_row_i0[k + 1] 
                             + xb_row_i0[k + 2] * nzbxat_row_i0[k + 2] 
                             + xb_row_i0[k + 3] * nzbxat_row_i0[k + 3];
                triangle[1] += xb_row_i0[k + 0] * nzbxat_row_i1[k + 0] 
                             + xb_row_i0[k + 1] * nzbxat_row_i1[k + 1] 
                             + xb_row_i0[k + 2] * nzbxat_row_i1[k + 2] 
                             + xb_row_i0[k + 3] * nzbxat_row_i1[k + 3];
                triangle[2] += xb_row_i0[k + 0] * nzbxat_row_i2[k + 0] 
                             + xb_row_i0[k + 1] * nzbxat_row_i2[k + 1] 
                             + xb_row_i0[k + 2] * nzbxat_row_i2[k + 2] 
                             + xb_row_i0[k + 3] * nzbxat_row_i2[k + 3];
                triangle[3] += xb_row_i0[k + 0] * nzbxat_row_i3[k + 0] 
                             + xb_row_i0[k + 1] * nzbxat_row_i3[k + 1] 
                             + xb_row_i0[k + 2] * nzbxat_row_i3[k + 2] 
                             + xb_row_i0[k + 3] * nzbxat_row_i3[k + 3];

                triangle[4] += xb_row_i1[k + 0] * nzbxat_row_i1[k + 0] 
                             + xb_row_i1[k + 1] * nzbxat_row_i1[k + 1] 
                             + xb_row_i1[k + 2] * nzbxat_row_i1[k + 2] 
                             + xb_row_i1[k + 3] * nzbxat_row_i1[k + 3];
                triangle[5] += xb_row_i1[k + 0] * nzbxat_row_i2[k + 0] 
                             + xb_row_i1[k + 1] * nzbxat_row_i2[k + 1] 
                             + xb_row_i1[k + 2] * nzbxat_row_i2[k + 2] 
                            + xb_row_i1[k + 3] * nzbxat_row_i2[k + 3];
                triangle[6] += xb_row_i1[k + 0] * nzbxat_row_i3[k + 0] 
                             + xb_row_i1[k + 1] * nzbxat_row_i3[k + 1] 
                             + xb_row_i1[k + 2] * nzbxat_row_i3[k + 2] 
                             + xb_row_i1[k + 3] * nzbxat_row_i3[k + 3];

                triangle[7] += xb_row_i2[k + 0] * nzbxat_row_i2[k + 0] 
                             + xb_row_i2[k + 1] * nzbxat_row_i2[k + 1] 
                             + xb_row_i2[k + 2] * nzbxat_row_i2[k + 2] 
                             + xb_row_i2[k + 3] * nzbxat_row_i2[k + 3];
                triangle[8] += xb_row_i2[k + 0] * nzbxat_row_i3[k + 0] 
                             + xb_row_i2[k + 1] * nzbxat_row_i3[k + 1] 
                             + xb_row_i2[k + 2] * nzbxat_row_i3[k + 2] 
                             + xb_row_i2[k + 3] * nzbxat_row_i3[k + 3];

                triangle[9] += xb_row_i3[k + 0] * nzbxat_row_i3[k + 0] 
                             + xb_row_i3[k + 1] * nzbxat_row_i3[k + 1] 
                             + xb_row_i3[k + 2] * nzbxat_row_i3[k + 2] 
                             + xb_row_i3[k + 3] * nzbxat_row_i3[k + 3];
            }
            for (unsigned k = hk; k < nk; k++) {
                triangle[0] += xb_row_i0[k] * nzbxat_row_i0[k];
                triangle[1] += xb_row_i0[k] * nzbxat_row_i1[k];
                triangle[2] += xb_row_i0[k] * nzbxat_row_i2[k];
                triangle[3] += xb_row_i0[k] * nzbxat_row_i3[k];

                triangle[4] += xb_row_i1[k] * nzbxat_row_i1[k];
                triangle[5] += xb_row_i1[k] * nzbxat_row_i2[k];
                triangle[6] += xb_row_i1[k] * nzbxat_row_i3[k];

                triangle[7] += xb_row_i2[k] * nzbxat_row_i2[k];
                triangle[8] += xb_row_i2[k] * nzbxat_row_i3[k];

                triangle[9] += xb_row_i3[k] * nzbxat_row_i3[k];

            }

            vektor<datatype> & row_i0 = rows[i + 0];
            vektor<datatype> & row_i1 = rows[i + 1];
            vektor<datatype> & row_i2 = rows[i + 2];
            vektor<datatype> & row_i3 = rows[i + 3];
            const vektor<datatype> & ya_row_i0 = c.rows[i + 0];
            const vektor<datatype> & ya_row_i1 = c.rows[i + 1];
            const vektor<datatype> & ya_row_i2 = c.rows[i + 2];
            const vektor<datatype> & ya_row_i3 = c.rows[i + 3];

            row_i0[i + 0] = -triangle[0] + ya_row_i0[i + 0];
            row_i0[i + 1] = -triangle[1] + ya_row_i0[i + 1];
            row_i0[i + 2] = -triangle[2] + ya_row_i0[i + 2];
            row_i0[i + 3] = -triangle[3] + ya_row_i0[i + 3];
            
            row_i1[i + 1] = -triangle[4] + ya_row_i1[i + 1];
            row_i1[i + 2] = -triangle[5] + ya_row_i1[i + 2];
            row_i1[i + 3] = -triangle[6] + ya_row_i1[i + 3];

            row_i2[i + 2] = -triangle[7] + ya_row_i2[i + 2];
            row_i2[i + 3] = -triangle[8] + ya_row_i2[i + 3];

            row_i3[i + 3] = -triangle[9] + ya_row_i3[i + 3];

            for (unsigned j = i + 4; j < hj; j += 4) {
                const vektor<datatype> & nzbxat_row_j0 = b.rows[j + 0];
                const vektor<datatype> & nzbxat_row_j1 = b.rows[j + 1];
                const vektor<datatype> & nzbxat_row_j2 = b.rows[j + 2];
                const vektor<datatype> & nzbxat_row_j3 = b.rows[j + 3];
                datatype square[16] = { datatype() };
                for (unsigned k = 0; k < hk; k += 4) {
                    square[0] += xb_row_i0[k + 0] * nzbxat_row_j0[k + 0] 
                               + xb_row_i0[k + 1] * nzbxat_row_j0[k + 1] 
                               + xb_row_i0[k + 2] * nzbxat_row_j0[k + 2] 
                               + xb_row_i0[k + 3] * nzbxat_row_j0[k + 3];
                    square[1] += xb_row_i0[k + 0] * nzbxat_row_j1[k + 0] 
                               + xb_row_i0[k + 1] * nzbxat_row_j1[k + 1] 
                               + xb_row_i0[k + 2] * nzbxat_row_j1[k + 2] 
                               + xb_row_i0[k + 3] * nzbxat_row_j1[k + 3];
                    square[2] += xb_row_i0[k + 0] * nzbxat_row_j2[k + 0] 
                               + xb_row_i0[k + 1] * nzbxat_row_j2[k + 1] 
                               + xb_row_i0[k + 2] * nzbxat_row_j2[k + 2] 
                               + xb_row_i0[k + 3] * nzbxat_row_j2[k + 3];
                    square[3] += xb_row_i0[k + 0] * nzbxat_row_j3[k + 0] 
                               + xb_row_i0[k + 1] * nzbxat_row_j3[k + 1] 
                               + xb_row_i0[k + 2] * nzbxat_row_j3[k + 2] 
                               + xb_row_i0[k + 3] * nzbxat_row_j3[k + 3];

                    square[4] += xb_row_i1[k + 0] * nzbxat_row_j0[k + 0] 
                               + xb_row_i1[k + 1] * nzbxat_row_j0[k + 1]
                               + xb_row_i1[k + 2] * nzbxat_row_j0[k + 2]
                               + xb_row_i1[k + 3] * nzbxat_row_j0[k + 3];
                    square[5] += xb_row_i1[k + 0] * nzbxat_row_j1[k + 0]
                               + xb_row_i1[k + 1] * nzbxat_row_j1[k + 1]
                               + xb_row_i1[k + 2] * nzbxat_row_j1[k + 2]
                               + xb_row_i1[k + 3] * nzbxat_row_j1[k + 3];
                    square[6] += xb_row_i1[k + 0] * nzbxat_row_j2[k + 0]
                               + xb_row_i1[k + 1] * nzbxat_row_j2[k + 1]
                               + xb_row_i1[k + 2] * nzbxat_row_j2[k + 2]
                               + xb_row_i1[k + 3] * nzbxat_row_j2[k + 3];
                    square[7] += xb_row_i1[k + 0] * nzbxat_row_j3[k + 0]
                               + xb_row_i1[k + 1] * nzbxat_row_j3[k + 1]
                               + xb_row_i1[k + 2] * nzbxat_row_j3[k + 2]
                               + xb_row_i1[k + 3] * nzbxat_row_j3[k + 3];

                    square[8] += xb_row_i2[k + 0] * nzbxat_row_j0[k + 0] 
                               + xb_row_i2[k + 1] * nzbxat_row_j0[k + 1]
                               + xb_row_i2[k + 2] * nzbxat_row_j0[k + 2]
                               + xb_row_i2[k + 3] * nzbxat_row_j0[k + 3];
                    square[9] += xb_row_i2[k + 0] * nzbxat_row_j1[k + 0]
                               + xb_row_i2[k + 1] * nzbxat_row_j1[k + 1]
                               + xb_row_i2[k + 2] * nzbxat_row_j1[k + 2]
                               + xb_row_i2[k + 3] * nzbxat_row_j1[k + 3];
                    square[10]+= xb_row_i2[k + 0] * nzbxat_row_j2[k + 0]
                               + xb_row_i2[k + 1] * nzbxat_row_j2[k + 1]
                               + xb_row_i2[k + 2] * nzbxat_row_j2[k + 2]
                               + xb_row_i2[k + 3] * nzbxat_row_j2[k + 3];
                    square[11]+= xb_row_i2[k + 0] * nzbxat_row_j3[k + 0]
                               + xb_row_i2[k + 1] * nzbxat_row_j3[k + 1]
                               + xb_row_i2[k + 2] * nzbxat_row_j3[k + 2]
                               + xb_row_i2[k + 3] * nzbxat_row_j3[k + 3];

                    square[12]+= xb_row_i3[k + 0] * nzbxat_row_j0[k + 0] 
                               + xb_row_i3[k + 1] * nzbxat_row_j0[k + 1]
                               + xb_row_i3[k + 2] * nzbxat_row_j0[k + 2]
                               + xb_row_i3[k + 3] * nzbxat_row_j0[k + 3];
                    square[13]+= xb_row_i3[k + 0] * nzbxat_row_j1[k + 0]
                               + xb_row_i3[k + 1] * nzbxat_row_j1[k + 1]
                               + xb_row_i3[k + 2] * nzbxat_row_j1[k + 2]
                               + xb_row_i3[k + 3] * nzbxat_row_j1[k + 3];
                    square[14]+= xb_row_i3[k + 0] * nzbxat_row_j2[k + 0]
                               + xb_row_i3[k + 1] * nzbxat_row_j2[k + 1]
                               + xb_row_i3[k + 2] * nzbxat_row_j2[k + 2]
                               + xb_row_i3[k + 3] * nzbxat_row_j2[k + 3];
                    square[15]+= xb_row_i3[k + 0] * nzbxat_row_j3[k + 0]
                               + xb_row_i3[k + 1] * nzbxat_row_j3[k + 1]
                               + xb_row_i3[k + 2] * nzbxat_row_j3[k + 2]
                               + xb_row_i3[k + 3] * nzbxat_row_j3[k + 3];
                }
                for (unsigned k = hk; k < nk; k++) {
                    square[0]  += xb_row_i0[k] * nzbxat_row_j0[k];
                    square[1]  += xb_row_i0[k] * nzbxat_row_j1[k];
                    square[2]  += xb_row_i0[k] * nzbxat_row_j2[k];
                    square[3]  += xb_row_i0[k] * nzbxat_row_j3[k];

                    square[4]  += xb_row_i1[k] * nzbxat_row_j0[k];
                    square[5]  += xb_row_i1[k] * nzbxat_row_j1[k];
                    square[6]  += xb_row_i1[k] * nzbxat_row_j2[k];
                    square[7]  += xb_row_i1[k] * nzbxat_row_j3[k];

                    square[8]  += xb_row_i2[k] * nzbxat_row_j0[k];
                    square[9]  += xb_row_i2[k] * nzbxat_row_j1[k];
                    square[10] += xb_row_i2[k] * nzbxat_row_j2[k];
                    square[11] += xb_row_i2[k] * nzbxat_row_j3[k];

                    square[12] += xb_row_i3[k] * nzbxat_row_j0[k];
                    square[13] += xb_row_i3[k] * nzbxat_row_j1[k];
                    square[14] += xb_row_i3[k] * nzbxat_row_j2[k];
                    square[15] += xb_row_i3[k] * nzbxat_row_j3[k];
                }

                row_i0[j + 0] = -square[0]  + ya_row_i0[j + 0];
                row_i0[j + 1] = -square[1]  + ya_row_i0[j + 1];
                row_i0[j + 2] = -square[2]  + ya_row_i0[j + 2];
                row_i0[j + 3] = -square[3]  + ya_row_i0[j + 3];

                row_i1[j + 0] = -square[4]  + ya_row_i1[j + 0];
                row_i1[j + 1] = -square[5]  + ya_row_i1[j + 1];
                row_i1[j + 2] = -square[6]  + ya_row_i1[j + 2];
                row_i1[j + 3] = -square[7]  + ya_row_i1[j + 3];

                row_i2[j + 0] = -square[8]  + ya_row_i2[j + 0];
                row_i2[j + 1] = -square[9]  + ya_row_i2[j + 1];
                row_i2[j + 2] = -square[10] + ya_row_i2[j + 2];
                row_i2[j + 3] = -square[11] + ya_row_i2[j + 3];

                row_i3[j + 0] = -square[12] + ya_row_i3[j + 0];
                row_i3[j + 1] = -square[13] + ya_row_i3[j + 1];
                row_i3[j + 2] = -square[14] + ya_row_i3[j + 2];
                row_i3[j + 3] = -square[15] + ya_row_i3[j + 3];
            }
            for (unsigned j = hj; j < nj; j++) {
                const vektor<datatype> & nzbxat_row_j0 = b.rows[j];
                datatype quad[4] = { datatype() };
                for (unsigned k = 0; k < nk; k++) {
                    quad[0] += xb_row_i0[k] * nzbxat_row_j0[k];
                    quad[1] += xb_row_i1[k] * nzbxat_row_j0[k];
                    quad[2] += xb_row_i2[k] * nzbxat_row_j0[k];
                    quad[3] += xb_row_i3[k] * nzbxat_row_j0[k];
                }
                row_i0[j] = -quad[0] + ya_row_i0[j];
                row_i1[j] = -quad[1] + ya_row_i1[j];
                row_i2[j] = -quad[2] + ya_row_i2[j];
                row_i3[j] = -quad[3] + ya_row_i3[j];
            }
	    }
        for (unsigned i = hi; i < ni; i++) {
            const vektor<datatype> & xb_row_i = a.rows[i];
            for (unsigned j = i; j < nj; j++) {
                const vektor<datatype> & nzbxat_sor_j = b.rows[j];
                datatype sum = datatype();
                for (unsigned k = 0; k < nk; k++) {
                    sum += xb_row_i[k] * nzbxat_sor_j[k];
                }
                rows[i][j] = -sum + c.rows[i][j];
            }
        }
        if(is_symmetrize_needed)
            symmetrize_from_upper();
    }
};


}

#endif