/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/perl/Ext.h"
#include <libxml/parser.h>

static SV* cur_path = NULL;

static
void restore_loader(pTHX_ void* p)
{
   AV* path_av=(AV*)SvRV(cur_path);
   I32 tail=AvFILLp(path_av)+1;
   xmlExternalEntityLoader std_loader=(xmlExternalEntityLoader)AvARRAY(path_av)[tail];
   SvREFCNT_dec(cur_path);
   cur_path=NULL;
   xmlSetExternalEntityLoader(std_loader);
}

/* copied with slight adaptations from libxslt/xsltproc.c, GNU XSLT processing library */

static
xmlParserInputPtr 
path_loader(const char *URL, const char *ID, xmlParserCtxtPtr ctxt)
{
   xmlParserInputPtr ret;
   warningSAXFunc warning = NULL;
   I32 i;

   AV* path_av=(AV*)SvRV(cur_path);
   I32 tail=AvFILLp(path_av)+1;
   xmlExternalEntityLoader std_loader=(xmlExternalEntityLoader)AvARRAY(path_av)[tail];

   if ((ctxt != NULL) && (ctxt->sax != NULL)) {
      warning = ctxt->sax->warning;
      ctxt->sax->warning = NULL;
   }

   ret = std_loader(URL, ID, ctxt);
   if (ret != NULL) {
      if (warning != NULL)
         ctxt->sax->warning = warning;
      return ret;
   }

   if (URL != NULL) {
      dTHX;
      const char *last_slash=strrchr(URL, '/');

      for (i=0; i<tail; ++i) {
         SV* newURL=newSVsv(AvARRAY(path_av)[i]);
         if (last_slash != NULL)
            sv_catpvn(newURL, last_slash, strlen(last_slash));
         else
            sv_catpvf(newURL, "/%s", URL);
         ret = std_loader(SvPVX(newURL), ID, ctxt);
         SvREFCNT_dec(newURL);
         if (ret != NULL) {
            if (warning != NULL)
               ctxt->sax->warning = warning;
            return ret;
         }
      }
   }

   if (warning != NULL) {
      ctxt->sax->warning = warning;
      if (URL != NULL)
         warning(ctxt, "failed to load external entity \"%s\"\n", URL);
      else if (ID != NULL)
         warning(ctxt, "failed to load external entity ID=\"%s\"\n", ID);
   }
   return NULL;
}

MODULE = Polymake::Core::XMLhandler           PACKAGE = Polymake::Core::XMLhandler

void
set_search_path(path)
   SV *path;
PROTOTYPE: $
PPCODE:
{
   AV* path_av=(AV*)SvRV(path);
   I32 tail=AvFILLp(path_av)+1;
   av_extend(path_av,tail);
   cur_path=SvREFCNT_inc_simple_NN(path);
   AvARRAY(path_av)[tail]=(SV*)xmlGetExternalEntityLoader();
   xmlSetExternalEntityLoader(&path_loader);
   LEAVE;
   save_destructor_x(&restore_loader, NULL);
   ENTER;
}

BOOT:
if (PL_DBgv) {
   CvNODEBUG_on(get_cv("Polymake::Core::XMLhandler::set_search_path", FALSE));
}

=pod
// Local Variables:
// mode:C
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
=cut
