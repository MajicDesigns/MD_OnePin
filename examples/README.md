# MD_OnePin Library Examples

If you like and use this library please consider making a small donation 
using [PayPal](https://paypal.me/MajicDesigns/4USD)

[Library Documentation](https://majicdesigns.github.io/MD_OnePin/)

<hr>

**Test\MD_OnePin_Test_Pri**  
Testing harness Primary node. Enter commands in the Serial Monitor to 
exercise the library and test the link. See the help text displayed 
in the Serial Monitor window for available commands.

This sketch is for use with the corresponding MD_OnePin_Test_Sec sketch
<hr>

**Test\MD_OnePin_Test_Sec**  
Testing harness Secondary node. This sketch is designed to work and provide
appropriate responses for the MD_OnePin_Test_Pri sketch.
<hr>

**MD_OnePin_Simple_Pri**  
The simplest Primary node sketch possible. This does nothing but could be 
a start for more complex applications.
<hr>

**MD_OnePin_Sec_C_LED**  
An example sketch written in C that implements an ISR to detect the message 
start, with the rest of the message processed in the loop() function.

The application flashes a LED at a rate determined by an internal variable.
Data writes from the PRI will set a new rate in milliseconds for the flash 
rate. Data Reads from the PRI will cause the SEC to send the current flash 
count.

The MD_OnePin_Test_Pri example can be used to write and read this SEC node.
<hr>

**MD_OnePin_SEC_ATTiny_LED**  
A derivative of the MD_OnePin_Sec_C_LED example tailored to and tested on 
ATTiny 13 and 85 processors. This example contains hardware specific 
calls for direct I/O manipulation to speed processing and reduce code 
size.
<hr>
