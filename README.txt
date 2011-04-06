This project contains the source code for a swarm of robots to synchronise flashing with each other, using e-puck robots (www.e-puck.org).

CONTENTS
firefly.hex		-	a pre-compiled version of the source code, ready for uploading to an e-puck.
epfl/			-	directory containing epuck library code developed by EPFL for use with the epucks. This is NOT the full library, only the files needed to compile this project.
firefly.c		-	source code for making the robots flash together
firefly.mcp		-	the MPLAB workspace with the build path and source code all set up.

The source code can be read and compiled using MPLAB and Microchip's C18 compiler, or a dsPIC IDE and compilation toolchain of your choice. 
Using MPLAB you can open firefly.mcp and click the "make all" button to compile this code.

This project is an implementation of the algorithm described in the following publication:
Christensen A., O'Grady R., and Dorigo M. (2009) "From fireflies to fault-tolerant swarms of robots", In: Transactions on Evolutionary Computation, volume 13(4) pp754-766. August 2009.


    Firely inspired E-Puck flashing demonstration.
    Copyright (C) 2011  Jennifer Owen (University of York)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	


