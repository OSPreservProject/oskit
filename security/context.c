/* FLASK */

/*
 * Copyright (c) 1999, 2000 The University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * Contributed by the Computer Security Research division,
 * INFOSEC Research and Technology Office, NSA.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include "context.h"

/* moved from context.h */

#ifdef CONFIG_FLASK_MLS
int mls_context_cpy(context_struct_t * dst, context_struct_t * src)
{
        (dst)->range.level[0].sens = (src)->range.level[0].sens;
        if (!ebitmap_cpy(&(dst)->range.level[0].cat, &(src)->range.level[0].cat)
)
                return -ENOMEM;
        (dst)->range.level[1].sens = (src)->range.level[1].sens;
        if (!ebitmap_cpy(&(dst)->range.level[1].cat, &(src)->range.level[1].cat)
) {
                ebitmap_destroy(&(dst)->range.level[0].cat);
                return -ENOMEM;
        }
        return 0;
}
#endif

int context_cpy(context_struct_t * dst, context_struct_t * src)
{
        (dst)->user = (src)->user;
        (dst)->role = (src)->role;
        (dst)->type = (src)->type;
        return mls_context_cpy(dst, src);
}

/* FLASK */
