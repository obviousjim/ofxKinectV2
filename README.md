ofxKinectV2
===========

This is an openFrameworks addon for working with the Microsoft SDK for the Kinect V2. There's a few caveats: you need a fairly specific setup to get this working. You'll need

* A computer with a USB 3 port (I'm on a MacBook Bro Retina mid 2012)
* Windows 8.1 x64
* [Visual Studio 2012](http://www.microsoft.com/en-us/download/details.aspx?id=34673)
* [A Kinect V2](http://www.microsoft.com/en-us/kinectforwindows/Purchase/developer-sku.aspx)
* The Kinect V2 SDK (will be released to the public in July)
* [openFrameworks with all the poco libs compiled for 2013](https://github.com/liquidzym/openFrameworks/tree/VS2013)
* 

even though the bropenFramework listed says it's compiled for 2013 you need Visual Studio 2012 Express for Windows Desktop.

* clone the oF repo recursively "git clone --recursive <repo>"
* then clone this add on into the addons folder
* open up in VS2012
* change the solution "startup project" to example-Simple so it doesn't try and run openFrameworks
* click on the projects inside the solution and then hit the wrench and change the Platform Toolset to Visual Studio 2012 (v110)
* 

This should now compile and run. The hand gesture data is printed out in the terminal.
