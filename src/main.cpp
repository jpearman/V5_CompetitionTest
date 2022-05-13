/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       james                                                     */
/*    Created:      Wed Apr 06 2022                                           */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#include "vex.h"
#include "comp_debug.h"

using namespace vex;

//#define BAD_CODE    1
#define GOOD_CODE   1

// A global instance of vex::brain used for printing to the V5 brain screen
vex::brain       Brain;
vex::competition Competition;
vex::controller  Controller;
vex::motor       m1(PORT1);

// debug class
competition_debug Cdebug( Competition );

/*----------------------------------------------------------------------------*/

void button_callback() {
    static  int calls = 0;
    Brain.Screen.setFont( mono40 );

    Brain.Screen.printAt( 100, 160, "Button %4d", ++calls );
}

/*----------------------------------------------------------------------------*/

void pre_auton( void ) {
    this_thread::sleep_for(100);
} 

/*----------------------------------------------------------------------------*/

void autonomous( void ) {
    vex::timer    runtime;

    Brain.Screen.clearScreen();
    Brain.Screen.setFont( mono40 );

    while(true) {
      Brain.Screen.printAt( 100, 100, "Auton  %7.2f", runtime.time()/1000.0 );
      this_thread::sleep_for(10);
    }
} 

/*----------------------------------------------------------------------------*/

void usercontrol( void ) {
    vex::timer    runtime;

  // incorrect placement of button event handler
#ifdef BAD_CODE
    Controller.ButtonA.pressed( button_callback );
#endif

    Brain.Screen.clearScreen();
    Brain.Screen.setFont( mono40 );

    while (true) {
      Brain.Screen.printAt( 100, 100, "Driver %7.2f", runtime.time()/1000.0 );
      m1.spin( forward, Controller.Axis1.position(), percent );
      this_thread::sleep_for(10);
    }
}

/*----------------------------------------------------------------------------*/

int main() {    
#ifdef GOOD_CODE
    Controller.ButtonA.pressed( button_callback );
#endif

    Competition.autonomous( autonomous );
    Competition.drivercontrol( usercontrol );

    pre_auton();

    while(1) {
        this_thread::sleep_for(10);
    }
}

