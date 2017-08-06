#include <iostream>

using namespace std;

//converting a signal flow to another
//by offset, intervall, etc

/*
int linear_signal_convert
this is a linear one. i think it's ok for this time.
- data : the input we have to convert by another interval's parameters
- offset : obvi. intervals maybe ofsetted from eachother
- data_range : the original range
- range : the new interval's range
- invert : obvi. maybe one interval's data type have to be inverted in incrementation
*/
int linear_signal_convert(const float data, const long offset, const long data_range, const long range, bool invert=false)
{
  int result=0;
    result = (int)((float)(data + offset)/(float)data_range*range);
    //cout << "LSC: data: " << data << " offset: " << offset << " data_range: " << data_range << " range: " << range << endl;
    //cout << "LSC_result: " << result << endl;
    if(result > range) {result = range;}
    if(invert==true){result=range-result;}
    if(result<0){result=0;}
    return result;
}
//maybe I shall make scale classes with cross-comparing methods and several incrementation parameters
//like exponential scale or else
