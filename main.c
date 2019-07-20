/*
 * main.c
 * 
 * Copyright 2019 Unknown <marco@portatile_arch>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include "common.h"
#include "tcpHandler.h"
#include "cliTasks.h"

int main(int argc, char **argv)
{
	CLI_Arguments arguments;
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	CLI_printArguments (arguments);
	
	return 0;
}

