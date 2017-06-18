/***************************************************************************
 * THIS DOCUMENTATION IS INCOMPLETE, BECAUSE I DON'T FULLY UNDERSTAND
 * THE ALGORITHM
 *
 * A meta-programming implementation of Batcher's Merge-Exchange sort
 * which constructs an efficient sorting network, for a fixed input
 * size, at compile time. Batcher's Merge-Exchange is not commonly
 * used for sorting networks, since the resultant network doesn't
 * allow for as many parallel operations as some other constructions,
 * and networks are designed for chips or parallel sorting. Batcher's
 * Merge-Exchange does however often produce fewer total comparisons.
 * There are a number of applications that need to very efficiently
 * sort 5, 7, or 32 numbers many, many, times and this code aims to,
 * at compile time, generate a near minimum set of swaps to accomplish
 * that task. Depending on compiler/system/optimizations/..., this
 * code is faster than the stl sort for N < ~(40-50).
 *
 * Batcher's Merge-Exchange sort is decribed in "The Art of Computer
 * Programming, Volume 3", by D.E.Knuth, on page 111. Oddly, every
 * reference I find to Batcher's Merge-Exchange points back to this
 * book and this page. On this page, the great and powerful Knuth
 * says, "[Batcher's Merge-Exchange] is not at all obvious; in fact,
 * it requires a fairly intricate proof just to show that it is valid,
 * since comparatively few comparisons are made". This makes me feel
 * a little better for not understanding exactly how or why this
 * algorithm works. But I will return to fix up this documentation
 * as soon as I figure it out!
 *
 * For now, I will use the algorithm outlined by Knuth, which consists
 * of 6 phases:
 *     M1: [Initialize p.] Set p<-2^{t-1}, where t=ceil(log_2(N)) is
 *         the least integer such that 2^t>=N. (Steps M2 through M5
 *         will be performed for p=2^{t-1}, 2^{t-2}, ..., 1.)
 *     M2: [Initialize q, r, d.] Set q<-2^{t-1}, r<-0, d<-p.
 *     M3: [Loop on i.] For all i such that 0<=i<N-d and i&p=r, do
 *         step M4. Then go to step M5. (Here i&p means the "logical and"
 *         of the binary representation of i and p; each bit of the
 *         result is zero except where both i and p have 1 bits in
 *         corresponding positions.)
 *     M4: [Compare/exchange X_{i+1}:X_{i+d+1}.] If X_{i+1}>X_{i+d+1},
 *         interchange X_{i+1}<->X_{i+d+1}.
 *     M5: [Loop on q.] If q!=p, set d<-q-p, q<-q/2, r<-p, and return
 *         to M3.
 *     M6: [Loop on p.] (At this point the permutation X_1, X_2, ..., X_N)
 *         is p-ordered.) Set p<-floor(p/2). If p>0, go back to M2.
 *
 * The psuedocode looks something like:
 *     t = ceil(log(N, 2))                              # Helper for M1, M2
 *     p = 1 << (t - 1)                                 # M1
 *     do:                                              # M6
 *         q = 1 << (t - 1)                             # M2
 *         r = 0                                        # M2
 *         d = p                                        # M2
 *         while(d > 0):                                # M5 (checking d instead of p and q to avoid goto)
 *             for i in 0->(N - d):                     # M3
 *                 if((i & p) == r):                    # M4
 *                     CompareExchange(i, i + d)        # M4 (0-indexing)
 *                 d = q - p                            # M5
 *                 q /= 2                               # M5
 *                 r = p                                # M5
 *         p /= 2                                       # M6
 *     while(p > 0);                                    # M6
 */

#ifndef NETWORKSORT_H
#define NETWORKSORT_H

#include <algorithm>    // For swap, sort
#include <functional>   // For less
#include <iterator>     // For iterator_traits
#include <type_traits>  // For conditional

/**
 * Function: NetworkSort<N>(RandomIterator begin);
 * ------------------------------------------------------------------------
 * Given an iterator to the range [begin, begin + N), sorts the elements
 * in that range using a network sort.
 */
template <size_t N, typename RandomIterator>
void NetworkSort(RandomIterator begin);

/**
 * Function: NetworkSort<N>(RandomIterator begin, Comparator comp);
 * -----------------------------------------------------------------------
 * Given an iterator to the range [begin, begin + N) and a comparator,
 * sorts the elements in that range into ascending order according to
 * comp.
 */
template <size_t N, typename RandomIterator, typename Comparator>
void NetworkSort(RandomIterator begin, Comparator comp);

/* * * * * Implementation Below This Point * * * * */

namespace details {

/**
 * Metafunction: Log2<i>
 * ---------------------------------------------------------------------
 * Returns ceil(lg(i))
 */
template <size_t i>
struct Log2 {
  static const size_t value = 1 + Log2<(i >> 1)>::value;
};

template <>
struct Log2<1> {
  static const size_t value = 0;
};
template <>
struct Log2<0> {
  static const size_t value = 0;
};

/**
 * Metafunction: CeilLog2<i>
 * ---------------------------------------------------------------------
 * Returns ceil(lg i) + 1;
 */
template <size_t i>
struct CeilLog2 {
  static const size_t value = Log2<i>::value + 1;
};

/**
 * Metaaction: NoOp
 * ---------------------------------------------------------------------
 * A do-nothing action.
 */
struct NoOp {
  static void action(auto&&, auto&&) {}
};

/**
 * Metaaction: SWAP<i, j>
 * --------------------------------------------------------------------
 * Ensures that the elements at array positions i and j are in relative
 * sorted order. It is assumed that i < j.
 */
template <typename RandomIterator, typename Comparator, size_t i, size_t j>
struct SWAP {
  static void action(RandomIterator array, Comparator comp) {
    /* Exchange the two if the latter is smaller than the former. */
    if (comp(array[j], array[i])) {
      std::swap(array[i], array[j]);
    }
  }
};

/**
 * Metaaction: __M4<i, p, r, d>
 * ----------------------------------------------------------------------
 * if((i & p) == r):
 *     CompareExchange(i, i + d)
 */
template <typename RandomIterator, typename Comparator, size_t i, size_t p,
          size_t r, size_t d>
struct __M4 {
  static void action(RandomIterator array, Comparator comp) {
    std::conditional<((i & p) == r), SWAP<RandomIterator, Comparator, i, i + d>,
                     NoOp>::type::action(array, comp);
  }
};

/**
 * Metaaction: __M3<i, p, d, r>
 * ---------------------------------------------------------------------
 * for i in 0->(N - d):
 *     __M4
 */
template <typename RandomIterator, typename Comparator, size_t i, size_t p,
          size_t r, size_t d>
struct __M3 {
  static void action(RandomIterator array, Comparator comp) {
    __M3<RandomIterator, Comparator, i - 1, p, r, d>::action(array, comp);
    __M4<RandomIterator, Comparator, i, p, r, d>::action(array, comp);
  }
};
template <typename RandomIterator, typename Comparator, size_t p, size_t r,
          size_t d>
struct __M3<RandomIterator, Comparator, size_t(-1), p, r, d> {
  static void action(RandomIterator, Comparator) {}
};

/**
 * Metaaction: __M5<p, r, d, q, N>
 * --------------------------------------------------------------------
 * while(d > 0):
 *     __M3
 *     d = q - p
 *     q /= 2
 *     r = p
 */
template <typename RandomIterator, typename Comparator, size_t p, size_t r,
          size_t d, size_t q, size_t N>
struct __M5 {
  static void action(RandomIterator array, Comparator comp) {
    std::conditional<(N >= d),
                     __M3<RandomIterator, Comparator, N - d - 1, p, r, d>,
                     NoOp>::type::action(array, comp);
    __M5<RandomIterator, Comparator, p, p, q - p, q / 2, N>::action(array,
                                                                    comp);
  }
};
template <typename RandomIterator, typename Comparator, size_t p, size_t r,
          size_t q, size_t N>
struct __M5<RandomIterator, Comparator, p, r, 0, q, N> {
  static void action(RandomIterator, Comparator) {}
};

/**
 * Metaaction: __M6__M2<p, t, N>
 * --------------------------------------------------------------------
 * do:
 *     __M5
 * while(p > 0);
 */
template <typename RandomIterator, typename Comparator, size_t p, size_t t,
          size_t N>
struct __M6__M2 {
  static void action(RandomIterator array, Comparator comp) {
    __M5<RandomIterator, Comparator, p, 0, p, (1 << (t - 1)), N>::action(array,
                                                                         comp);
    __M6__M2<RandomIterator, Comparator, p / 2, t, N>::action(array, comp);
  }
};
template <typename RandomIterator, typename Comparator, size_t t, size_t N>
struct __M6__M2<RandomIterator, Comparator, 0, t, N> {
  static void action(RandomIterator, Comparator) {}
};

/**
 * Metaaction: STLSort<N>
 * --------------------------------------------------------------------
 * The effectiveness of this sorting network diminishes as the input
 * range grows too large. This action falls back on sorting the input
 * range using the STL's provided sort routine.
 */
template <typename RandomIterator, typename Comparator, size_t N>
struct STLSort {
  static void action(RandomIterator array, Comparator comp) {
    std::sort(array, array + N);
  }
};

/**
 * Metaaction: LowerBoundCheck<N>
 * ----------------------------------------------------------------------
 * Action that detects whether no action is necessary (i.e. if the input
 * range is empty) and fires off the sort otherwise.
 */
template <typename RandomIterator, typename Comparator, size_t N>
struct LowerBoundCheck {
  static void action(RandomIterator array, Comparator comp) {
    std::conditional<
        (N <= 1), NoOp,
        __M6__M2<RandomIterator, Comparator, (1 << (CeilLog2<N>::value - 1)),
                 CeilLog2<N>::value, N>>::type::action(array, comp);
  }
};

/**
 * Metaaction: UpperBoundCheck<N>
 * ----------------------------------------------------------------------
 * Determines whether to use a network sort or the built-in STL sort to
 * sort the input. The crossover point of which is more efficient is
 * dependent on the system and options. It is usually around 50.
 */
template <typename RandomIterator, typename Comparator, size_t N>
struct UpperBoundCheck {
  static void action(RandomIterator array, Comparator comp) {
    std::conditional<
        (N > 64), STLSort<RandomIterator, Comparator, N>,
        LowerBoundCheck<RandomIterator, Comparator, N>>::type::action(array,
                                                                      comp);
  }
};

/**
 * Metaaction: NetworkSort<N>
 * ---------------------------------------------------------------------
 * Action that fires off a network sort.
 */
template <typename RandomIterator, typename Comparator, size_t N>
struct NetworkSort {
  static void sort(RandomIterator array, Comparator comp) {
    UpperBoundCheck<RandomIterator, Comparator, N>::action(array, comp);
  }
};

/*
 * Optimal sorting networks are known for N<=8 and the network produced
 * by Batcher's Merge-Exchange matches the number of comparisons. For
 * 9<=N<=16, networks with slightly fewer comparisons are known. Since
 * this code is meant to be efficient, those networks have been hard-
 * coded.
 */

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 9> {  // R. W. Floyd.
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 10> {  // A. Waksman.
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 4, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 11> {
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 12> {  // Shapiro and Green.
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 13> {
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 1, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 14> {
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 12, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 15> {
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 12, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 12, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 13, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
  }
};

template <typename RandomIterator, typename Comparator>
struct NetworkSort<RandomIterator, Comparator, 16> {  // Green.
  static void sort(RandomIterator array, Comparator comp) {
    SWAP<RandomIterator, Comparator, 0, 1>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 12, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 14, 15>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 12, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 3>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 13, 15>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 15>::action(array, comp);
    SWAP<RandomIterator, Comparator, 0, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 15>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 13, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 11>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 2>::action(array, comp);
    SWAP<RandomIterator, Comparator, 4, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 1, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 14>::action(array, comp);
    SWAP<RandomIterator, Comparator, 2, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 13>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 10, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 5>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 9>::action(array, comp);
    SWAP<RandomIterator, Comparator, 3, 4>::action(array, comp);
    SWAP<RandomIterator, Comparator, 5, 6>::action(array, comp);
    SWAP<RandomIterator, Comparator, 7, 8>::action(array, comp);
    SWAP<RandomIterator, Comparator, 9, 10>::action(array, comp);
    SWAP<RandomIterator, Comparator, 11, 12>::action(array, comp);
    SWAP<RandomIterator, Comparator, 6, 7>::action(array, comp);
    SWAP<RandomIterator, Comparator, 8, 9>::action(array, comp);
  }
};
}

/* Actual implementation of NetworkSort. */
template <size_t i, typename RandomIterator, typename Comparator>
void NetworkSort(RandomIterator begin, Comparator comp) {
  details::NetworkSort<RandomIterator, Comparator, i>::sort(begin, comp);
}

/* Non-comparator version calls comparator version with the default comparator.
 */
template <size_t i, typename RandomIterator>
void NetworkSort(RandomIterator begin) {
  NetworkSort<i>(
      begin,
      std::less<typename std::iterator_traits<RandomIterator>::value_type>());
}

#endif  // NETWORKSORT_H

