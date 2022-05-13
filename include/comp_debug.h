/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    MIT License                                                             */
/*                                                                            */
/*    Copyright (c) 2022 James Pearman                                        */
/*                                                                            */
/*    Permission is hereby granted, free of charge, to any person obtaining a */
/*    copy of this software and associated documentation files (the           */
/*    "Software"), to deal in the Software without restriction, including     */
/*    without limitation the rights to use, copy, modify, merge, publish,     */
/*    distribute, sublicense, and/or sell copies of the Software, and to      */
/*    permit persons to whom the Software is furnished to do so, subject to   */
/*    the following conditions:                                               */
/*                                                                            */
/*    The above copyright notice and this permission notice shall be included */
/*    in all copies or substantial portions of the Software.                  */
/*                                                                            */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS */
/*    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              */
/*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  */
/*    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    */
/*    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    */
/*    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       */
/*    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE                   */
/*                                                                            */
/*    Module:     comp_debug.h                                                */
/*    Created:    6 April 2022                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef   COMP_DEBUG_CLASS_H
#define   COMP_DEBUG_CLASS_H

#include "v5.h"
#include "v5_cpp.h"

// I'm using a couple of private APIs to allow this code to work with most
// VEXcode user programs and over lay the information and thread count.
extern "C" { 
  void vexDisplayClipRegionSetWithIndex( int32_t index, int32_t x1, int32_t y1, int32_t x2, int32_t y2 );
  void *vexTaskGetCallbackAndId( uint32_t index, int *callback_id );
}

class competition_debug {
    private:
      vex::thread       _t1;    // our update thread
      vex::competition *_c1;    // pointer to user code competition class instance
      vex::brain::lcd   _lcd;   // local instance of brain::lcd for drawing on the V5 screen

      enum class tCompStateType {
        kCompStateUnknown = 0,
        kCompStateDisabled,
        kCompStateDriver,
        kCompStateAuton
      };

      // current state of competition event triggers
      tCompStateType    _state = tCompStateType::kCompStateUnknown;

    public:
      competition_debug( vex::competition &c ) {
        // pointer to competition instance
        _c1 = &c;
        // create thread to handle status display
        _t1 = vex::thread( status_loop, static_cast<void *>(this) );
      }

      ~competition_debug(){
        // stop our update thread
        _t1.interrupt();
      };

      // the update thread needs to be a statuc member function, we pass pointer to
      // class instance as argument
      static int status_loop( void *arg  ) {
          if( arg == NULL )
            return(0);

          competition_debug *instance = static_cast<competition_debug *>(arg);

          vex::devices  devs;
          vex::timer    t1;
          vex::color    currentStatusColor = vex::color(0x000000);
          vex::color    newStatusColor     = vex::color(0x808080);
          vex::color    textColor          = vex::color(0x000000);
          int loop_ctr = 0;
          
          // get maximum number of tasks we support
          int max_tasks = vex::thread::hardware_concurrency();
          
          // set clip region for all tasks, we need to use a private API for this.
          // It stops other tasks that, for example, may clear the screen from removing
          // the competition status.
          //
          for ( int i=0; i<max_tasks;  i++ ) {
            vexDisplayClipRegionSetWithIndex( i, 10, 25, 470, 230);
          }

          // loop forever monitoring competition status
          while(1) {
            if( currentStatusColor != newStatusColor ) {
              instance->_lcd.setClipRegion(0, 0, 480, 240);
              instance->_lcd.setPenWidth(10);
              instance->_lcd.setPenColor(newStatusColor);
              instance->_lcd.drawRectangle(5, 10, 465, 220, vex::transparent);
              instance->_lcd.setPenWidth(1);
              instance->_lcd.drawRectangle(0, 0, 480, 25, newStatusColor);
              instance->_lcd.setFillColor(newStatusColor);
              currentStatusColor = newStatusColor;
              loop_ctr = 0;
            }
          
            // limit drawing to the top 25 rows of pixels
            instance->_lcd.setClipRegion(0, 0, 480, 25);
            instance->_lcd.setPenColor(textColor);

            bool bCompetition = instance->_c1->isCompetitionSwitch() || instance->_c1->isFieldControl();
            bool bRadioAvailable = (devs.numberOf(V5_DeviceType::kDeviceTypeRadioSensor) != 0);

            // show radio status
            if( !bRadioAvailable ) {
              instance->_lcd.printAt( 380, 20, "No Radio " );
            }
            else {
              instance->_lcd.printAt( 380, 20, "         " );
            }

            if( loop_ctr-- <= 0 ) {
              loop_ctr = 10;
              int nt = count_threads();
              instance->_lcd.printAt( 220, 20, "Threads %3d", nt );
            }

            // Update status
            switch( instance->_state ) {
              default:
                instance->_lcd.printAt( 10, 20, "Comp Not Activated  " );
                break;
              case tCompStateType::kCompStateDisabled:
                instance->_lcd.printAt( 10, 20, "Disabled            " );
                break;
              case tCompStateType::kCompStateDriver:
                instance->_lcd.printAt( 10, 20, "Driver     %7.2f", t1.time()/1000.0 );
                break;
              case tCompStateType::kCompStateAuton:
                instance->_lcd.printAt( 10, 20, "Autonomous %7.2f", t1.time()/1000.0 );
                break;
            }

            // monitor competition state changes
            // these are mevents that latch
            if( instance->_c1->DISABLED ) {
              newStatusColor = vex::red;
              textColor = vex::yellow;
              instance->_state = tCompStateType::kCompStateDisabled;
            }
            else
            if( instance->_c1->DRIVER_CONTROL ) {
              newStatusColor = vex::green;
              textColor = vex::black;
              instance->_state = tCompStateType::kCompStateDriver;
              t1.clear();
            }
            else
            if( instance->_c1->AUTONOMOUS ) {
              newStatusColor = vex::blue;
              textColor = vex::white;
              instance->_state = tCompStateType::kCompStateAuton;
              t1.clear();
            }

            vex::this_thread::sleep_for(25);
          }

          return 0;        
      }

      static int count_threads() {
        int     callback_id = 0;

        // get maximum number of tasks we support
        int max_tasks = vex::thread::hardware_concurrency();
        int n = 0;

        for( int i=0;i<max_tasks;i++) {
          void * callback = (void *)vexTaskGetCallbackAndId(i, &callback_id);
          if( callback != NULL ) {
            n++;
          }
        }

        return n;
      }
};

#endif  // COMP_DEBUG_CLASS_H
