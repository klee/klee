/******************************************************************************************[Sort.h]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Sort_h
#define Sort_h


namespace MINISAT {
//=================================================================================================


template<class T>
struct LessThan_default {
    bool operator () (T x, T y) { return x < y; }
};


//=================================================================================================


template <class T, class LessThan>
void selectionSort(T* array, int size, LessThan lt)
{
    int     i, j, best_i;
    T       tmp;

    for (i = 0; i < size-1; i++){
        best_i = i;
        for (j = i+1; j < size; j++){
            if (lt(array[j], array[best_i]))
                best_i = j;
        }
        tmp = array[i]; array[i] = array[best_i]; array[best_i] = tmp;
    }
}
template <class T> static inline void selectionSort(T* array, int size) {
    selectionSort(array, size, LessThan_default<T>()); }


template <class T, class LessThan>
void sort(T* array, int size, LessThan lt, double& seed)
{
    if (size <= 15)
        selectionSort(array, size, lt);

    else{
        T           pivot = array[irand(seed, size)];
        T           tmp;
        int         i = -1;
        int         j = size;

        for(;;){
            do i++; while(lt(array[i], pivot));
            do j--; while(lt(pivot, array[j]));

            if (i >= j) break;

            tmp = array[i]; array[i] = array[j]; array[j] = tmp;
        }

        sort(array    , i     , lt, seed);
        sort(&array[i], size-i, lt, seed);
    }
}
template <class T, class LessThan> void sort(T* array, int size, LessThan lt) {
    double  seed = 91648253; sort(array, size, lt, seed); }
template <class T> static inline void sort(T* array, int size) {
    sort(array, size, LessThan_default<T>()); }


template <class T, class LessThan>
void sortUnique(T* array, int& size, LessThan lt)
{
    int         i, j;
    T           last;

    if (size == 0) return;

    sort(array, size, lt);

    i    = 1;
    last = array[0];
    for (j = 1; j < size; j++){
        if (lt(last, array[j])){
            last = array[i] = array[j];
            i++; }
    }

    size = i;
}
template <class T> static inline void sortUnique(T* array, int& size) {
    sortUnique(array, size, LessThan_default<T>()); }


//=================================================================================================
// For 'vec's:


template <class T, class LessThan> void sort(vec<T>& v, LessThan lt) {
    sort((T*)v, v.size(), lt); }
template <class T> void sort(vec<T>& v) {
    sort(v, LessThan_default<T>()); }


template <class T, class LessThan> void sortUnique(vec<T>& v, LessThan lt) {
    int     size = v.size();
    T*      data = v.release();
    sortUnique(data, size, lt);
    v.~vec<T>();
    new (&v) vec<T>(data, size); }
template <class T> void sortUnique(vec<T>& v) {
    sortUnique(v, LessThan_default<T>()); }


//=================================================================================================
};
#endif
