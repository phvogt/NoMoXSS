/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:expandtab:shiftwidth=4:tabstop=4:
 */
/* vim:set ts=8 sw=2 et cindent: */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developers of the Original Code are Christopher
 * Blizzard and Jamie Zawinski.  Portions created Christopher Blizzard
 * are Copyright (C) 2000 Christopher Blizzard.  Portions created by
 * Jamie Zawinski are Copyright (C) 1994 Netscape Communications
 * Corporation.  All Rights Reserved.
 * 
 * Contributor(s): 
 */

#include "XRemoteClient.h"
#include "prmem.h"
#include "prprf.h"
#include "plstr.h"
#include "prsystem.h"
#include "prlog.h"
#include "prenv.h"
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xatom.h>
#ifdef POLL_WITH_XCONNECTIONNUMBER
#include <poll.h>
#endif

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
#define MOZILLA_USER_PROP      "_MOZILLA_USER"
#define MOZILLA_PROFILE_PROP   "_MOZILLA_PROFILE"
#define MOZILLA_PROGRAM_PROP   "_MOZILLA_PROGRAM"

static PRLogModuleInfo *sRemoteLm = NULL;

XRemoteClient::XRemoteClient()
{
  mDisplay = 0;
  mInitialized = PR_FALSE;
  mMozVersionAtom = 0;
  mMozLockAtom = 0;
  mMozCommandAtom = 0;
  mMozResponseAtom = 0;
  mMozWMStateAtom = 0;
  mMozUserAtom = 0;
  mLockData = 0;
  if (!sRemoteLm)
    sRemoteLm = PR_NewLogModule("XRemoteClient");
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::XRemoteClient"));
}

XRemoteClient::~XRemoteClient()
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::~XRemoteClient"));
  if (mInitialized)
    Shutdown();
}

#ifndef XREMOTE_STANDALONE
NS_IMPL_ISUPPORTS1(XRemoteClient, nsIXRemoteClient)
#endif

NS_IMETHODIMP
XRemoteClient::Init (void)
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::Init"));

  if (mInitialized)
    return NS_OK;
  // try to open the display
  mDisplay = XOpenDisplay(0);
  if (!mDisplay)
    return NS_ERROR_FAILURE;

  // get our atoms
  mMozVersionAtom  = XInternAtom(mDisplay, MOZILLA_VERSION_PROP, False);
  mMozLockAtom     = XInternAtom(mDisplay, MOZILLA_LOCK_PROP, False);
  mMozCommandAtom  = XInternAtom(mDisplay, MOZILLA_COMMAND_PROP, False);
  mMozResponseAtom = XInternAtom(mDisplay, MOZILLA_RESPONSE_PROP, False);
  mMozWMStateAtom  = XInternAtom(mDisplay, "WM_STATE", False);
  mMozUserAtom     = XInternAtom(mDisplay, MOZILLA_USER_PROP, False);
  mMozProfileAtom  = XInternAtom(mDisplay, MOZILLA_PROFILE_PROP, False);
  mMozProgramAtom  = XInternAtom(mDisplay, MOZILLA_PROGRAM_PROP, False);

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
XRemoteClient::Shutdown (void)
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::Shutdown"));

  if (!mInitialized)
    return NS_OK;
  // shut everything down
  XCloseDisplay(mDisplay);
  mDisplay = 0;
  mInitialized = PR_FALSE;
  if (mLockData) {
    free(mLockData);
    mLockData = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP
XRemoteClient::SendCommand (const char *aProgram, const char *aUsername,
                            const char *aProfile, const char *aCommand,
                            char **aResponse, PRBool *aWindowFound)
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::SendCommand"));

  *aWindowFound = PR_FALSE;

  Window w = FindBestWindow(aProgram, aUsername, aProfile);

  nsresult rv = NS_OK;

  if (w) {
    // ok, let the caller know that we at least found a window.
    *aWindowFound = PR_TRUE;

    // make sure we get the right events on that window
    XSelectInput(mDisplay, w,
                 (PropertyChangeMask|StructureNotifyMask));
        
    PRBool destroyed = PR_FALSE;

    // get the lock on the window
    rv = GetLock(w, &destroyed);

    if (NS_SUCCEEDED(rv)) {
      // send our command
      rv = DoSendCommand(w, aCommand, aResponse, &destroyed);

      // if the window was destroyed, don't bother trying to free the
      // lock.
      if (!destroyed)
          FreeLock(w); // doesn't really matter what this returns

    }
  }

  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("SendCommand returning 0x%x\n", rv));

  return rv;
}

Window
XRemoteClient::CheckWindow(Window aWindow)
{
  Atom type = None;
  int  format;
  unsigned long nitems, bytesafter;
  unsigned char *data;
  Window innerWindow;

  XGetWindowProperty(mDisplay, aWindow, mMozWMStateAtom,
		     0, 0, False, AnyPropertyType,
		     &type, &format, &nitems, &bytesafter, &data);

  if (type)
    return aWindow;

  // didn't find it here so check the children of this window
  innerWindow = CheckChildren(aWindow);

  if (innerWindow)
    return innerWindow;

  return aWindow;
}

Window
XRemoteClient::CheckChildren(Window aWindow)
{
  Window root, parent;
  Window *children;
  unsigned int nchildren;
  unsigned int i;
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data;
  Window retval = None;
  
  if (!XQueryTree(mDisplay, aWindow, &root, &parent, &children,
		  &nchildren))
    return None;
  
  // scan the list first before recursing into the list of windows
  // which can get quite deep.
  for (i=0; !retval && (i < nchildren); i++) {
    XGetWindowProperty(mDisplay, children[i], mMozWMStateAtom,
		       0, 0, False, AnyPropertyType, &type, &format,
		       &nitems, &after, &data);
    if (type)
      retval = children[i];
  }

  // otherwise recurse into the list
  for (i=0; !retval && (i < nchildren); i++) {
    retval = CheckChildren(children[i]);
  }

  if (children)
    XFree((char *)children);

  return retval;
}

nsresult
XRemoteClient::GetLock(Window aWindow, PRBool *aDestroyed)
{
  PRBool locked = PR_FALSE;
  PRBool waited = PR_FALSE;
  *aDestroyed = PR_FALSE;

  if (!mLockData) {
    
    char pidstr[32];
    char sysinfobuf[SYS_INFO_BUFFER_LENGTH];
    PR_snprintf(pidstr, sizeof(pidstr), "pid%d@", getpid());
    PRStatus status;
    status = PR_GetSystemInfo(PR_SI_HOSTNAME, sysinfobuf,
			      SYS_INFO_BUFFER_LENGTH);
    if (status != PR_SUCCESS) {
      return NS_ERROR_FAILURE;
    }
    
    // allocate enough space for the string plus the terminating
    // char
    mLockData = (char *)malloc(strlen(pidstr) + strlen(sysinfobuf) + 1);
    if (!mLockData)
      return NS_ERROR_OUT_OF_MEMORY;

    strcpy(mLockData, pidstr);
    if (!strcat(mLockData, sysinfobuf))
      return NS_ERROR_FAILURE;
  }

  do {
    int result;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = 0;

    XGrabServer(mDisplay);

    result = XGetWindowProperty (mDisplay, aWindow, mMozLockAtom,
				 0, (65536 / sizeof (long)),
				 False, /* don't delete */
				 XA_STRING,
				 &actual_type, &actual_format,
				 &nitems, &bytes_after,
				 &data);
    if (result != Success || actual_type == None) {
      /* It's not now locked - lock it. */
      XChangeProperty (mDisplay, aWindow, mMozLockAtom, XA_STRING, 8,
		       PropModeReplace,
		       (unsigned char *)mLockData,
		       strlen(mLockData));
      locked = True;
    }

    XUngrabServer(mDisplay);
    XSync(mDisplay, False);

    if (!locked) {
      /* We tried to grab the lock this time, and failed because someone
	 else is holding it already.  So, wait for a PropertyDelete event
	 to come in, and try again. */
      PR_LOG(sRemoteLm, PR_LOG_DEBUG, 
	     ("window 0x%x is locked by %s; waiting...\n",
	      (unsigned int) aWindow, data));
      waited = True;
      while (1) {
	XEvent event;
	int select_retval;
#ifdef POLL_WITH_XCONNECTIONNUMBER
       struct pollfd fds[1];
       fds[0].fd = XConnectionNumber(mDisplay);
       fds[0].events = POLLIN;
       select_retval = poll(fds,1,10*1000);
#else
	fd_set select_set;
	struct timeval delay;
	delay.tv_sec = 10;
	delay.tv_usec = 0;

	FD_ZERO(&select_set);
	// add the x event queue to the select set
	FD_SET(ConnectionNumber(mDisplay), &select_set);
	select_retval = select(ConnectionNumber(mDisplay) + 1,
			       &select_set, NULL, NULL, &delay);
#endif
	// did we time out?
	if (select_retval == 0) {
	  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("timed out waiting for window\n"));
	  return NS_ERROR_FAILURE;
	}
	PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("xevent...\n"));
	XNextEvent (mDisplay, &event);
	if (event.xany.type == DestroyNotify &&
	    event.xdestroywindow.window == aWindow) {
	  PR_LOG(sRemoteLm, PR_LOG_DEBUG,
		 ("window 0x%x unexpectedly destroyed.\n",
		  (unsigned int) aWindow));
	  *aDestroyed = PR_TRUE;
	  return NS_ERROR_FAILURE;
	}
	else if (event.xany.type == PropertyNotify &&
		 event.xproperty.state == PropertyDelete &&
		 event.xproperty.window == aWindow &&
		 event.xproperty.atom == mMozLockAtom) {
	  /* Ok!  Someone deleted their lock, so now we can try
	     again. */
	  PR_LOG(sRemoteLm, PR_LOG_DEBUG,
		 ("(0x%x unlocked, trying again...)\n",
		  (unsigned int) aWindow));
		  break;
	}
      }
    }
    if (data)
      XFree(data);
  } while (!locked);

  if (waited) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("obtained lock.\n"));
  }

  return NS_OK;
}

Window
XRemoteClient::FindBestWindow(const char *aProgram, const char *aUsername,
                              const char *aProfile)
{
  Window root = RootWindowOfScreen(DefaultScreenOfDisplay(mDisplay));
  Window bestWindow = 0;
  Window root2, parent, *kids;
  unsigned int nkids;
  int i;

  // Get a list of the children of the root window, walk the list
  // looking for the best window that fits the criteria.
  if (!XQueryTree(mDisplay, root, &root2, &parent, &kids, &nkids)) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG,
           ("XQueryTree failed in XRemoteClient::FindBestWindow"));
    return 0;
  }

  if (!(kids && nkids)) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("root window has no children"));
    return 0;
  }

  // We'll walk the list of windows looking for a window that best
  // fits the criteria here.

  for (i=nkids-1; i >= 0; i--) {
    Atom type;
    int format;
    unsigned long nitems, bytesafter;
    unsigned char *data_return = 0;
    Window w;
    w = kids[i];
    // find the inner window with WM_STATE on it
    w = CheckWindow(w);

    int status = XGetWindowProperty(mDisplay, w, mMozVersionAtom,
                                    0, (65536 / sizeof (long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);

    if (!data_return)
      continue;

    XFree(data_return);
    data_return = 0;

    if (status != Success || type == None)
      continue;

    // If someone passed in a program name, check it against this one
    // unless it's "any" in which case, we don't care.  If someone did
    // pass in a program name and this window doesn't support that
    // protocol, we don't include it in our list.
    if (aProgram && strcmp(aProgram, "any")) {
        status = XGetWindowProperty(mDisplay, w, mMozProgramAtom,
                                    0, (65536 / sizeof(long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);
        
        // If the return name is not the same as what someone passed in,
        // we don't want this window.
        if (data_return) {
            if (strcmp(aProgram, (const char *)data_return)) {
                XFree(data_return);
                continue;
            }

            // This is actually the success condition.
            XFree(data_return);
        }
        else {
            // Doesn't support the protocol, even though the user
            // requested it.  So we're not going to use this window.
            continue;
        }
    }

    // Check to see if it has the user atom on that window.  If there
    // is then we need to make sure that it matches what we have.
    const char *username;
    if (aUsername) {
      username = aUsername;
    }
    else {
      username = PR_GetEnv("LOGNAME");
    }

    if (username) {
        status = XGetWindowProperty(mDisplay, w, mMozUserAtom,
                                    0, (65536 / sizeof(long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);

        // if there's a username compare it with what we have
        if (data_return) {
            // If the IDs aren't equal, we don't want this window.
            if (strcmp(username, (const char *)data_return)) {
                XFree(data_return);
                continue;
            }

            XFree(data_return);
        }
    }

    // Check to see if there's a profile name on this window.  If
    // there is, then we need to make sure it matches what someone
    // passed in.
    if (aProfile) {
        status = XGetWindowProperty(mDisplay, w, mMozProfileAtom,
                                    0, (65536 / sizeof(long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);

        // If there's a profile compare it with what we have
        if (data_return) {
            // If the profiles aren't equal, we don't want this window.
            if (strcmp(aProfile, (const char *)data_return)) {
                XFree(data_return);
                continue;
            }

            XFree(data_return);
        }
    }

    // If we got this far, this is the best window so far.  It passed
    // all the tests.
    bestWindow = w;
  }

  return bestWindow;
}

nsresult
XRemoteClient::FreeLock(Window aWindow)
{
  int result;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = 0;

  result = XGetWindowProperty(mDisplay, aWindow, mMozLockAtom,
                              0, (65536 / sizeof(long)),
                              True, /* atomic delete after */
                              XA_STRING,
                              &actual_type, &actual_format,
                              &nitems, &bytes_after,
                              &data);
  if (result != Success) {
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             ("unable to read and delete " MOZILLA_LOCK_PROP
              " property\n"));
      return NS_ERROR_FAILURE;
  }
  else if (!data || !*data){
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             ("invalid data on " MOZILLA_LOCK_PROP
              " of window 0x%x.\n",
              (unsigned int) aWindow));
      return NS_ERROR_FAILURE;
  }
  else if (strcmp((char *)data, mLockData)) {
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             (MOZILLA_LOCK_PROP " was stolen!  Expected \"%s\", saw \"%s\"!\n",
              mLockData, data));
      return NS_ERROR_FAILURE;
  }

  if (data)
      XFree(data);
  return NS_OK;
}

nsresult
XRemoteClient::DoSendCommand(Window aWindow, const char *aCommand,
                             char **aResponse, PRBool *aDestroyed)
{
  PRBool done = PR_FALSE;
  PRBool accepted = PR_FALSE;
  *aDestroyed = PR_FALSE;

  PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	 ("(writing " MOZILLA_COMMAND_PROP " \"%s\" to 0x%x)\n",
	  aCommand, (unsigned int) aWindow));

  XChangeProperty (mDisplay, aWindow, mMozCommandAtom, XA_STRING, 8,
		   PropModeReplace, (unsigned char *)aCommand,
		   strlen(aCommand));

  while (!done) {
    XEvent event;
    XNextEvent (mDisplay, &event);
    if (event.xany.type == DestroyNotify &&
	event.xdestroywindow.window == aWindow) {
      /* Print to warn user...*/
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	     ("window 0x%x was destroyed.\n",
	      (unsigned int) aWindow));
      *aResponse = strdup("Window was destroyed while reading response.");
      *aDestroyed = PR_TRUE;
      goto DONE;
    }
    else if (event.xany.type == PropertyNotify &&
	     event.xproperty.state == PropertyNewValue &&
	     event.xproperty.window == aWindow &&
	     event.xproperty.atom == mMozResponseAtom) {
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      unsigned char *data = 0;
      Bool result;
      result = XGetWindowProperty (mDisplay, aWindow, mMozResponseAtom,
				   0, (65536 / sizeof (long)),
				   True, /* atomic delete after */
				   XA_STRING,
				   &actual_type, &actual_format,
				   &nitems, &bytes_after,
				   &data);
      if (result != Success) {
	PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	       ("failed reading " MOZILLA_RESPONSE_PROP
		" from window 0x%0x.\n",
		(unsigned int) aWindow));
    *aResponse = strdup("Internal error reading response from window.");
	done = PR_TRUE;
      }
      else if (!data || strlen((char *) data) < 5) {
	PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	       ("invalid data on " MOZILLA_RESPONSE_PROP
		" property of window 0x%0x.\n",
		(unsigned int) aWindow));
    *aResponse = strdup("Server returned invalid data in response.");
	done = PR_TRUE;
      }
      else if (*data == '1') {	/* positive preliminary reply */
	PR_LOG(sRemoteLm, PR_LOG_DEBUG,  ("%s\n", data + 4));
	/* keep going */
	done = PR_FALSE;
      }

      else if (!strncmp ((char *)data, "200", 3)) { /* positive completion */
	*aResponse = strdup((char *)data);
	accepted = PR_TRUE;
	done = PR_TRUE;
      }

      else if (*data == '2') {	/* positive completion */
	PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("%s\n", data + 4));
    *aResponse = strdup((char *)data);
	accepted = PR_TRUE;
	done = PR_TRUE;
      }

      else if (*data == '3') {	/* positive intermediate reply */
	PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	       ("internal error: "
		"server wants more information?  (%s)\n",
		data));
	*aResponse = strdup((char *)data);
	done = PR_TRUE;
      }

      else if (*data == '4' ||	/* transient negative completion */
	       *data == '5') {	/* permanent negative completion */
	PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("%s\n", data + 4));
	*aResponse = strdup((char *)data);
	done = PR_TRUE;
      }

      else {
	PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	       ("unrecognised " MOZILLA_RESPONSE_PROP
		" from window 0x%x: %s\n",
		(unsigned int) aWindow, data));
	*aResponse = strdup((char *)data);
	done = PR_TRUE;
      }

      if (data)
	XFree(data);
    }

    else if (event.xany.type == PropertyNotify &&
	     event.xproperty.window == aWindow &&
	       event.xproperty.state == PropertyDelete &&
	     event.xproperty.atom == mMozCommandAtom) {
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
	     ("(server 0x%x has accepted "
	      MOZILLA_COMMAND_PROP ".)\n",
	      (unsigned int) aWindow));
    }
    
  }

 DONE:

  if (!accepted)
    return NS_ERROR_FAILURE;
  
  return NS_OK;
}

  
