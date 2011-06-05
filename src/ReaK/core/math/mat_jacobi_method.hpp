
/*
 *    Copyright 2011 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAT_JACOBI_METHOD_HPP
#define MAT_JACOBI_METHOD_HPP

#include "mat_alg.hpp"
#include "mat_num_exceptions.hpp"


namespace ReaK {





/*************************************************************************
                      Jacobi Eigenvalue Algorithm
*************************************************************************/

namespace detail {



template <typename Matrix>
typename mat_traits<Matrix>::size_type jacobi_maxind(const Matrix& A, typename mat_traits<Matrix>::size_type k) {
  typedef typename mat_traits<Matrix>::size_type SizeType;
  using std::fabs;
  
  SizeType N = A.get_row_count();
  if(k >= N-1)
    return N-1;
  SizeType result(k+1);
  for(SizeType i=k+2;i<N;++i)
    if(fabs(A(k,i)) > fabs(A(k,result)))
      result = i;
  return result;
};

template <typename Matrix>
void jacobi_rotate(Matrix& A, 
		   typename mat_traits<Matrix>::size_type k,
		   typename mat_traits<Matrix>::size_type l,
		   typename mat_traits<Matrix>::size_type i,
		   typename mat_traits<Matrix>::size_type j,
		   typename mat_traits<Matrix>::value_type s,
		   typename mat_traits<Matrix>::value_type c) {
  typename mat_traits<Matrix>::value_type tmp1 = c * A(k,l) - s * A(i,j);
  A(i,j) = s * A(k,l) + c * A(i,j);
  A(k,l) = tmp1;
};

template <typename Matrix1, typename Matrix2, typename Matrix3>
void eigensolve_Jacobi_impl(Matrix1& A, Matrix2& E, Matrix3& Q, typename mat_traits<Matrix1>::value_type NumTol) {
  typedef typename mat_traits<Matrix1>::value_type ValueType; 
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  using std::fabs;
  using std::sqrt;
  
  SizeType N = A.get_row_count(); 
  
  // init lambda, Q, and arrays ind, changed
  vect_n<SizeType> ind(N);
  std::vector<bool> changed(N);
  for(SizeType i=0;i<N;++i) {
    for(SizeType j=i+1;j<N;++j)
      Q(j,i) = (Q(i,j) = 0.0);
    Q(i,i) = 1.0;
    E(i,i) = A(i,i);
    changed[i] = true;
  };
  for(SizeType i=0;i<N-1;++i)
    ind[i] = jacobi_maxind(A,i);
  SizeType state(N); 

  while(state != 0) { //next rotation
    //Find the maximum off-diagonal term
    SizeType m(0); 
    for(SizeType i=1;i<N-1;++i)
      if(fabs(A(i,ind[i])) > fabs(A(m,ind[m])))
	m = i;
    SizeType k(m); //Choose the row index of the pivot element.
    SizeType l(ind[m]); //Choose the column index of the pivot element.

    //calculate c = cos(phi), s = sin(phi)
    ValueType p = A(k,l);
    if(fabs(p) < NumTol)              // if Akl is below tolerance, then it is over because the maximum off-diagonal term
      return;                         // is almost zero (close enough to zero).
    ValueType y(0.5*(E(l,l) - E(k,k)));
    ValueType t(fabs(y) + sqrt(p*p + y*y));
    ValueType s(sqrt(p*p + t*t));
    ValueType c(t / s);
    s = p / s;
    t = p*p / t;
    if(y < 0.0) {
      s = -s;
      t = -t;
    };
    A(k,l) = 0.0; //Update off-diagonal
    E(k,k) -= t; //Update diagonal k
    E(l,l) += t; //Update diagonal l

    //Track the changes to the diagonal terms, state decreases as diagonal terms become numerically constant.
    if(fabs(t) < NumTol) {
      if(changed[k]) {
	changed[k] = false; --state; };
      if(changed[l]) {
	changed[l] = false; --state; };
    } else {
      if(!changed[k]) {
	changed[k] = true; ++state; };
      if(!changed[l]) {
	changed[l] = true; ++state; };
    };

    //Perform a Jacobi rotation on the symmetric matrix A.
    for(SizeType i=0;i<k;++i)
      jacobi_rotate(A,i,k,i,l,s,c);
    for(SizeType i=k+1;i<l;++i)
      jacobi_rotate(A,k,i,i,l,s,c);
    for(SizeType i=l+1;i<N;++i)
      jacobi_rotate(A,k,i,l,i,s,c);

    //Perform a Givens rotation on the eigen vector matrix.
    for(SizeType i=0;i<N;++i) {
      ValueType tmp1   = c * Q(i,k) - s * Q(i,l);
      Q(i,l) = s * Q(i,k) + c * Q(i,l);
      Q(i,k) = tmp1;
    };

    //Find the biggest off-diagonal elements in the two updated rows.
    ind[k] = jacobi_maxind(A,k);
    ind[l] = jacobi_maxind(A,l);
  };
};

};


/**
 * Computes the eigen-values / -vectors of a matrix via the Jacobi Algorithm.
 *
 * \param A real symmetric matrix.
 * \param E holds, as output, the unsorted eigenvalue on the diagonal.
 * \param Q holds as output, the eigenvectors corresponding to the list of eigenvalues in E.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2, typename Matrix3>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value &&
                             ((mat_traits<Matrix1>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix1>::structure == mat_structure::tridiagonal)) &&
                             is_writable_matrix<Matrix2>::value && 
                             (mat_traits<Matrix2>::structure == mat_structure::diagonal) &&
                             is_fully_writable_matrix<Matrix3>::value, 
void >::type eigensolve_Jacobi(const Matrix1& A, Matrix2& E, Matrix3& Q, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  mat<typename mat_traits<Matrix1>::value_type, mat_structure::square> S(A);
  E.set_col_count(A.get_col_count());
  Q.set_row_count(A.get_col_count());
  Q.set_col_count(A.get_col_count());
  detail::eigensolve_Jacobi_impl(S,E,Q,NumTol);
};

/**
 * Computes the eigen-values / -vectors of a matrix via the Jacobi Algorithm.
 *
 * \param A real symmetric matrix.
 * \param E holds, as output, the unsorted eigenvalue on the diagonal.
 * \param Q holds as output, the eigenvectors corresponding to the list of eigenvalues in E.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2, typename Matrix3>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value &&
                             ((mat_traits<Matrix1>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix1>::structure == mat_structure::tridiagonal)) &&
                             is_writable_matrix<Matrix2>::value && 
                             ((mat_traits<Matrix2>::structure == mat_structure::rectangular) ||
                              (mat_traits<Matrix2>::structure == mat_structure::square) ||
                              (mat_traits<Matrix2>::structure == mat_structure::tridiagonal) ||
                              (mat_traits<Matrix2>::structure == mat_structure::upper_triangular) ||
                              (mat_traits<Matrix2>::structure == mat_structure::lower_triangular) ||
                              (mat_traits<Matrix2>::structure == mat_structure::symmetric)) &&
                             is_fully_writable_matrix<Matrix3>::value, 
void >::type eigensolve_Jacobi(const Matrix1& A, Matrix2& E, Matrix3& Q, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  mat<typename mat_traits<Matrix1>::value_type, mat_structure::square> S(A);
  mat<typename mat_traits<Matrix2>::value_type, mat_structure::diagonal> E_tmp(A.get_col_count());
  Q.set_row_count(A.get_col_count());
  Q.set_col_count(A.get_col_count());
  detail::eigensolve_Jacobi_impl(S,E_tmp,Q,NumTol);
  E = E_tmp;
};


/**
 * Computes the eigen-values / -vectors of a matrix via the Jacobi Algorithm.
 *
 * \param A real symmetric matrix.
 * \param E holds, as output, the unsorted eigenvalue on the diagonal.
 * \param Q holds as output, the eigenvectors corresponding to the list of eigenvalues in E.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2, typename Matrix3>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value &&
                             ((mat_traits<Matrix1>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix1>::structure == mat_structure::tridiagonal)) &&
                             is_writable_matrix<Matrix2>::value && 
                             (mat_traits<Matrix2>::structure == mat_structure::diagonal) &&
                             is_writable_matrix<Matrix3>::value &&
                             (mat_traits<Matrix3>::structure == mat_structure::orthogonal), 
void >::type eigensolve_Jacobi(const Matrix1& A, Matrix2& E, Matrix3& Q, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  mat<typename mat_traits<Matrix1>::value_type, mat_structure::square> S(A);
  E.set_col_count(A.get_col_count());
  mat<typename mat_traits<Matrix3>::value_type, mat_structure::square> Q_tmp(A.get_col_count());
  detail::eigensolve_Jacobi_impl(S,E,Q_tmp,NumTol);
  Q = Q_tmp;
};

/**
 * Computes the eigen-values / -vectors of a matrix via the Jacobi Algorithm.
 *
 * \param A real symmetric matrix.
 * \param E holds, as output, the unsorted eigenvalue on the diagonal.
 * \param Q holds as output, the eigenvectors corresponding to the list of eigenvalues in E.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2, typename Matrix3>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value &&
                             ((mat_traits<Matrix1>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix1>::structure == mat_structure::tridiagonal)) &&
                             is_writable_matrix<Matrix2>::value && 
                             ((mat_traits<Matrix2>::structure == mat_structure::rectangular) ||
                              (mat_traits<Matrix2>::structure == mat_structure::square) ||
                              (mat_traits<Matrix2>::structure == mat_structure::tridiagonal) ||
                              (mat_traits<Matrix2>::structure == mat_structure::upper_triangular) ||
                              (mat_traits<Matrix2>::structure == mat_structure::lower_triangular) ||
                              (mat_traits<Matrix2>::structure == mat_structure::symmetric)) &&
                             is_fully_writable_matrix<Matrix3>::value &&
                             (mat_traits<Matrix3>::structure == mat_structure::orthogonal), 
void >::type eigensolve_Jacobi(const Matrix1& A, Matrix2& E, Matrix3& Q, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  mat<typename mat_traits<Matrix1>::value_type, mat_structure::square> S(A);
  mat<typename mat_traits<Matrix2>::value_type, mat_structure::diagonal> E_tmp(A.get_col_count());
  mat<typename mat_traits<Matrix3>::value_type, mat_structure::square> Q_tmp(A.get_col_count());
  detail::eigensolve_Jacobi_impl(S,E_tmp,Q_tmp,NumTol);
  E = E_tmp;
  Q = Q_tmp;
};


/**
 * Solves the linear least square problem (AX \approx B or X = min_X(||AX - B||)) via the Jacobi Algorithm.
 *
 * \param A real symmetric matrix.
 * \param x stores the solution matrix as output (ColCount x ColCount2).
 * \param b stores the RHS of the linear system of equation (RowCount x ColCount2).
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \throws std::range_error if the matrix b's row count does not equal that of A.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2, typename Matrix3>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value &&
                             ((mat_traits<Matrix1>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix1>::structure == mat_structure::tridiagonal)) &&
                             is_writable_matrix<Matrix2>::value &&
                             is_readable_matrix<Matrix3>::value , 
void >::type linlsq_Jacobi(const Matrix1& A, Matrix2& x, const Matrix3& b, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  if(A.get_row_count() != b.get_row_count())
    throw std::range_error("Linear Least-square solution is only possible if row count of b is equal to row count of A!");
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  using std::fabs;
  
  mat<ValueType,mat_structure::square> S(A);
  mat<ValueType,mat_structure::diagonal> E(A.get_row_count());
  mat<ValueType,mat_structure::square, mat_alignment::column_major> Q(A.get_row_count());
  detail::eigensolve_Jacobi_impl(S,E,Q,NumTol);
  for(SizeType i=0;i<A.get_row_count();++i)
    if(fabs(E(i,i)) > NumTol)
      E(i,i) = 1.0 / E(i,i);
  x = (Q * (E * (transpose(Q) * b)));
};


/**
 * Functor to wrap a call to a Jacobi-Method-based linear-least-square solver.
 */
struct Jacobi_linlsqsolver {
  template <typename Matrix1, typename Matrix2, typename Matrix3>
  void operator()(const Matrix1& A, Matrix2& X, const Matrix3& B, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
    linlsq_Jacobi(A,X,B,NumTol);
  };
};



/**
 * Computes the pseudo-inverse of a matrix via the Jacobi Algorithm.
 *
 * \param A real symmetric matrix to be inverted.
 * \param A_inv the pseudo-inverse of A.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value &&
                             ((mat_traits<Matrix1>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix1>::structure == mat_structure::tridiagonal)) &&
                             is_writable_matrix<Matrix2>::value,
void >::type pseudoinvert_Jacobi(const Matrix1& A, Matrix2& A_inv, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  mat<ValueType,mat_structure::square> S(A);
  mat<ValueType,mat_structure::diagonal> E(A.get_row_count());
  mat<ValueType,mat_structure::square, mat_alignment::column_major> Q(A.get_row_count()); 
  detail::eigensolve_Jacobi_impl(S,E,Q,NumTol); 
  for(SizeType i=0;i<A.get_row_count();++i)
    if(fabs(E(i,i)) > NumTol)
      E(i,i) = 1.0 / E(i,i);
    
  A_inv = Q * (E * transpose(Q)); 
};

/**
 * Computes the determinant of a matrix via the Jacobi Algorithm.
 * \param A real symmetric matrix for which the determinant is needed.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 * \return the determinant of A.
 *
 * \author Mikael Persson
 */
template <typename Matrix>
typename boost::enable_if_c< is_readable_matrix<Matrix>::value &&
                             ((mat_traits<Matrix>::structure == mat_structure::symmetric) ||
                              (mat_traits<Matrix>::structure == mat_structure::tridiagonal)),
typename mat_traits<Matrix>::value_type >::type determinant_Jacobi(const Matrix& A, typename mat_traits<Matrix>::value_type NumTol = 1E-8) {
  typedef typename mat_traits<Matrix>::value_type ValueType;
  typedef typename mat_traits<Matrix>::size_type SizeType;
  mat<ValueType,mat_structure::square> S(A);
  mat<ValueType,mat_structure::diagonal> E(A.get_row_count());
  mat<ValueType,mat_structure::square> Q(A.get_row_count());
  detail::eigensolve_Jacobi_impl(S,E,Q,NumTol);
  ValueType result(1.0);
  for(SizeType i=0;i<A.get_row_count();++i)
    result *= E(i,i);
  return result;
};






};

#endif







