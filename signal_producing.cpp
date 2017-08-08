#include <iostream>
#include <queue>
#include <unistd.h>
#include "rtmidi/RtMidi.h"

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
- increment : incrementation between scale values
*/
int linear_signal_convert(const float data, const long offset, const long data_range, const long range, bool invert=false, int increment=1)
{
  int result=0;
    result = (int)((float)(data + offset)/(float)data_range*range);
    //cout << "LSC: data: " << data << " offset: " << offset << " data_range: " << data_range << " range: " << range << endl;
    //cout << "LSC_result: " << result << endl;
    if(result > range) {result = range;}
    if(invert==true){result=range-result;}
    if(result<0){result=0;}
    result *= increment;
    return result;
}
//maybe I shall make scale classes with cross-comparing methods and several incrementation parameters
//like exponential scale or else

//------------------------------------------------------------------------------

//moving average

int moving_average(deque<int>& stack, int dataIn)
{
  int sum=0;
  stack.pop_front();
  stack.push_back(dataIn);
  for (int i = 0; i<stack.size();i++)
  {
    sum+=stack.at(i);
  }
  return sum/stack.size();
}

void init_stack(deque<int>& stack, int size)
{
  for (int i = 0; i<size;i++)
  {
    stack.push_back(0);
  }
}

//------------------------------------------------------------------------------

//MIDI signal session

//making midi NoteOn message

void MidiNote(unsigned char note, unsigned char velocity, RtMidiOut& midiout)
{
  vector<unsigned char> MIDImessage;
  MIDImessage.push_back(144); //NoteOn command
  MIDImessage.push_back(note);
  MIDImessage.push_back(velocity);
  midiout.sendMessage( &MIDImessage );
  usleep(50);
}

void terminateMidiNote(unsigned char note, unsigned char velocity, RtMidiOut& midiout)
{
  vector<unsigned char> MIDImessage;
  MIDImessage.push_back(128); //NoteOff command
  MIDImessage.push_back(note);
  MIDImessage.push_back(velocity);
  midiout.sendMessage( &MIDImessage );
  usleep(50);
}
