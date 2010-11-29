
# ####
#
#  This file is licensed from, and is a trade secret of:
#
#    Mercury Computer Systems, Incorporated
#    199 Riverneck Road
#    Chelmsford, Massachusetts 01824-2820
#    United States of America
#    Telephone + 1 978 256-1300
#    Telecopy/FAX + 1 978 256-35999
#    US Customer Support (800) 872-0040
#
#  Refer to your Software License Agreements for provisions on use,
#  duplication, third party rights and disclosure. This file: (a) was
#  developed at private expense, and no part was developed with government
#  funds, (b) is a trade secret of Mercury Computer Systems, Inc. for the
#  purposes of the Freedom of Information Act, (c) is "commercial computer
#  software" subject to limited utilization as provided in the above noted
#  Software License Agreements and (d) is in all respects data belonging
#  exclusively to either Mercury Computer Systems, Inc. or its third party
#  providers.
#
#  Copyright (c) Mercury Computer Systems, Inc., Chelmsford MA., 1984-2009,
#  and all third party embedded software sources. All rights reserved under
#  the Copyright laws of US. and international treaties.
#
########################################################################### #

TOOLS_PATH := $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

include $(TOOLS_PATH)common.mk

