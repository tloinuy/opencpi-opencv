
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

`include "ocpi_ocp_defs.v"
localparam OCPI_WCI_CONFIG =       1'b1;
localparam OCPI_WCI_CONTROL =      1'b0;
localparam OCPI_WCI_EXISTS =       0;
localparam OCPI_WCI_INITIALIZED =  1;
localparam OCPI_WCI_OPERATING =    2;
localparam OCPI_WCI_SUSPENDED =    3;
localparam OCPI_WCI_UNUSABLE =     4;
localparam OCPI_WCI_STATE_WIDTH =  3;
`define OCPI_WCI_STATE_RANGE       [OCPI_WIP_WCI_STATE_WIDTH-1:0]
localparam OCPI_WCI_INITIALIZE =   3'h0;
localparam OCPI_WCI_START =        3'h1;
localparam OCPI_WCI_STOP =         3'h2;
localparam OCPI_WCI_RELEASE =      3'h3;
localparam OCPI_WCI_TEST =         3'h4;
localparam OCPI_WCI_BEFORE_QUERY = 3'h5;
localparam OCPI_WCI_AFTER_CONFIG = 3'h6;
localparam OCPI_WCI_RESERVED =     3'h7;


