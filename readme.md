A libgphoto2 daemon interface for use with my Gphoto Webserver.

It is currently incomplete as the instruction processing system only works for fixed size data and it needs to work with strings. It should probably be converted to a text based system for easier integration with javascript. 

If you are looking to contribute or are future me, I'm sorry. The file you are looking for is "buffer.h" and you're going to need to write a better interface for it. 

If you have any questions, feel free to create an issue and I will respond. 

Currently it loads the camera config and then dumps it to a file.


Dependencies
libgphoto2, CTRE
git submodule can get you CTRE and use the gphoto2 package on ubuntu

Compile warning 
Format.h file is a bit template heavy.


Sorry about leaving this project as a bit of a mess, unfortunately life just got in the way.
