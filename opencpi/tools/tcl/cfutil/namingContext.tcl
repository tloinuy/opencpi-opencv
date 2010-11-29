
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

#
# ----------------------------------------------------------------------
# Implementation of the Naming Service's Naming Context API that waits
# for someone to bind to it.
# ----------------------------------------------------------------------
#

namespace eval cfutil {}

catch {itcl::delete class cfutil::WaitingNamingContext}

itcl::class cfutil::WaitingNamingContext {
    inherit PortableServer::ServantBase

    private variable m_expectedId
    public variable haveBinding
    public variable boundObject

    public method _Interface {} {
        return "IDL:omg.org/CosNaming/NamingContextExt:1.0"
    }

    constructor {expectedId} {
        set haveBinding 0
        set boundObject 0
        set m_expectedId $expectedId
    }

    destructor {
        corba::release $boundObject
    }

    public method bind {n obj} {
        if {[llength $n] != 1} {
            corba::throw IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0
        }

        array set nn [lindex $n 0]

        if {$nn(id) == $m_expectedId && $nn(kind) == ""} {
            if {$haveBinding} {
                corba::throw IDL:omg.org/CosNaming/NamingContext/AlreadyBound:1.0
            }

            set haveBinding 1
            set boundObject [corba::duplicate $obj]
        } else {
            corba::throw IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0
        }
    }

    public method rebind {n obj} {
        if {[llength $n] != 1} {
            corba::throw IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0
        }

        array set nn [lindex $n 0]

        if {$nn(id) == $m_expectedId && $nn(kind) == ""} {
            if {$haveBinding} {
                corba::release $boundObject
            }

            set haveBinding 1
            set boundObject [corba::duplicate $obj]
        } else {
            corba::throw IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0
        }
    }

    public method resolve {n} {
        if {[llength $n] != 1} {
            corba::throw IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0
        }

        array set nn [lindex $n 0]

        if {$nn(id) == $m_expectedId && $nn(kind) == "" && $haveBinding} {
            return [corba::duplicate $boundObject]
        } else {
            corba::throw IDL:omg.org/CosNaming/NamingContext/InvalidName:1.0
        }
    }

    public method to_name {sn} {
        set r [list]
        # Assumes no backslashes ...
        foreach ni [split $sn "/"] {
            if {[llength [set n [split $ni "."]]] == 1} {
                lappend r [list id [lindex $n 0] kind ""]
            } else {
                lappend r [list id [lindex $n 0] kind [lindex $n 1]]
            }
        }
        return $r
    }

    #
    # Don't bother implementing the remaining operations.
    #
}

#
# ----------------------------------------------------------------------
# Helper class to wait for a Name Service binding, making the above
# easier to use.
# ----------------------------------------------------------------------
#

catch {itcl::delete class cfutil::WaitForNameServiceBinding}

itcl::class cfutil::WaitForNameServiceBinding {
    private variable m_reference
    private variable m_servant
    private variable m_oid
    private variable m_poa
    private variable m_cond

    constructor {expectedId} {
        #
        # To keep the API simple, use the RootPOA.
        #

        set m_poa [corba::resolve_initial_references "RootPOA"]
        set mgr [$m_poa the_POAManager]
        $mgr activate

        #
        # Start our Naming Context.
        #

        set m_servant [namespace current]::[cfutil::WaitingNamingContext \#auto $expectedId]
        set m_oid [$m_poa activate_object $m_servant]
        set m_reference [$m_poa id_to_reference $m_oid]
    }

    destructor {
        $m_poa deactivate_object $m_oid
        corba::release $m_reference
        itcl::delete object $m_servant
    }

    public method getContext {} {
        return [corba::duplicate $m_reference]
    }

    public method waitForBinding {{timeout 60}} {
        for {set to 0} {$to < $timeout} {incr to} {
            if {[$m_servant cget -haveBinding]} {
                break
            }

            after 1000 [list set [itcl::scope m_cond] 1]
            vwait [itcl::scope m_cond]
        }

        return [$m_servant cget -haveBinding]
    }

    public method getBinding {} {
        return [corba::duplicate [$m_servant cget -boundObject]]
    }
}
