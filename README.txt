To test code. 
First it must be on Linux, probably Ubuntu.
Then run ./configure in the home directory of the repository after cloning. 
The data generator can be tested by  simply typing ./gen when in the corresponding folder. 
The GUI can be tested by typing python GUI.py in the home folder of the cloned repository. Be carefult with monitor flight, it will not work without the rocket on and transmitting. Logs canbe read, and sim can be read if docker is up. 
To start Docker go into data_handle and type ./start_docker.sh, to stop it type ./stop_docker.sh.
To run the most comprehensive test open two terminals, go into data_hendling on both. Use one to start docker and wait a few seconds for all parts to start. Then in the other terminal, type ./datahandle -sim. after this you can go to a web browser and go to localhost:8080. This will show you the web visualization on test data. You can also test the 3d trace by going into trace and typing ./trace -x 10 -z 10. This is to give it a good looking scaling in the 3D x-z plane. 
