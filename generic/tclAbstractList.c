/*
 * tclAbstractList.h --
 *
 *	The AbstractList Obj Type -- a psuedo List 
 *
 * Copyright © 2022 by Brian Griffin. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclAbstractList.h"


/* -------------------------- AbstractList object ---------------------------- */

/*
 * Prototypes for procedures defined later in this file:
 */

static void		DupAbstractListInternalRep (Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);
static void		FreeAbstractListInternalRep (Tcl_Obj *listPtr);
static int		SetAbstractListFromAny (Tcl_Interp *interp, Tcl_Obj *objPtr);
static void		UpdateStringOfAbstractList (Tcl_Obj *listPtr);

/*
 * The structure below defines the AbstractList Tcl object type by means of
 * procedures that can be invoked by generic object code.
 *
 * The abstract list object is a special case of Tcl list represented by a set
 * of functions.
 * 
 */

const Tcl_ObjType tclAbstractListType = {
    "abstractlist",			/* name */
    FreeAbstractListInternalRep,	/* freeIntRepProc */
    DupAbstractListInternalRep,		/* dupIntRepProc */
    UpdateStringOfAbstractList,		/* updateStringProc */
    SetAbstractListFromAny		/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AbstractListLen --
 *
 * 	Compute the length of the equivalent list
 *
 * Results:
 *
 * 	The length of the list generated by the given range,
 * 	that may be zero.
 *
 * Side effects:
 *
 * 	None.
 *
 *----------------------------------------------------------------------
 */
Tcl_WideInt
Tcl_AbstractListObjLength(Tcl_Obj *abstractListObjPtr)
{
    return AbstractListObjLength(abstractListObjPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NewAbstractListObj()
 *
 *	Creates a new ArithSeries object. The returned object has
 *	refcount = 0.
 *
 * Results:
 *
 * 	A Tcl_Obj pointer to the created ArithSeries object.
 * 	A NULL pointer of the range is invalid.
 *
 * Side Effects:
 *
 * 	None.
 *----------------------------------------------------------------------
 */

Tcl_Obj*
Tcl_NewAbstractListObj(Tcl_Interp *interp, const char* typeName, size_t requiredSize)
{
    Tcl_Obj *objPtr;
    size_t repSize;
    AbstractList *abstractListRepPtr;
    Tcl_ObjInternalRep itr;
    (void)interp;
    TclNewObj(objPtr);
    repSize = sizeof(AbstractList) + requiredSize;
    abstractListRepPtr = (AbstractList*)ckalloc(repSize);
    abstractListRepPtr->version = TCL_ABSTRACTLIST_VERSION_1;
    abstractListRepPtr->repSize = repSize;
    abstractListRepPtr->typeName = typeName;;
    abstractListRepPtr->elements = NULL;
    abstractListRepPtr->newObjProc = NULL;
    abstractListRepPtr->dupRepProc = NULL;
    abstractListRepPtr->lengthProc = NULL;
    abstractListRepPtr->indexProc = NULL;
    abstractListRepPtr->sliceProc = NULL;
    abstractListRepPtr->reverseProc = NULL;
    itr.twoPtrValue.ptr1 = abstractListRepPtr;
    itr.twoPtrValue.ptr2 = (((char*)abstractListRepPtr) + offsetof(AbstractList,abstractValue));
    Tcl_StoreInternalRep(objPtr, &tclAbstractListType, &itr);
    Tcl_InvalidateStringRep(objPtr);
    return objPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_AbstractListObjIndex --
 *
 *	Returns the element with the specified index in the list
 *	represented by the specified Abstract List object.
 *	If the index is out of range, TCL_ERROR is returned,
 *	otherwise TCL_OK is returned and the integer value of the
 *	element is stored in *element.
 *
 * Results:
 *
 * 	Element Tcl_Obj is returned on succes, NULL on index out of range.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj*
Tcl_AbstractListObjIndex(Tcl_Obj *abstractListObjPtr, Tcl_WideInt index)
{
    AbstractList *abstractListRepPtr;
    Tcl_Obj *elementObjPtr;
    if (abstractListObjPtr->typePtr != &tclAbstractListType) {
        Tcl_Panic("Tcl_AbstractListObjIndex called without and AbstractList Obj.");
    }
    abstractListRepPtr = (AbstractList*)
	    abstractListObjPtr->internalRep.twoPtrValue.ptr1;
    elementObjPtr = abstractListRepPtr->indexProc(abstractListObjPtr, index);

    return elementObjPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeAbstractListInternalRep --
 *
 *	Deallocate the storage associated with an abstract list object's
 *	internal representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees abstractListPtr's AbstractList* internal representation and
 *	sets listPtr's	internalRep.twoPtrValue.ptr1 to NULL.
 *
 *----------------------------------------------------------------------
 */

void
FreeAbstractListInternalRep(Tcl_Obj *abstractListObjPtr)
{
    AbstractList *abstractListRepPtr =
	(AbstractList *) abstractListObjPtr->internalRep.twoPtrValue.ptr1;
    if (abstractListRepPtr->elements) {
	Tcl_WideInt i, llen = abstractListRepPtr->lengthProc(abstractListObjPtr);
	for(i=0; i<llen; i++) {
	    if (abstractListRepPtr->elements[i]) {
		Tcl_DecrRefCount(abstractListRepPtr->elements[i]);
	    }
	}
	ckfree((char*)abstractListRepPtr->elements);
    }
    ckfree((char *) abstractListRepPtr);
    abstractListObjPtr->internalRep.twoPtrValue.ptr1 = NULL;
    abstractListObjPtr->internalRep.twoPtrValue.ptr2 = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupAbstractListInternalRep --
 *
 *	Initialize the internal representation of a AbstractList Tcl_Obj to a
 *	copy of the internal representation of an existing arithseries object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We set "copyPtr"s internal rep to a pointer to a
 *	newly allocated AbstractList structure.
 *----------------------------------------------------------------------
 */

static void
DupAbstractListInternalRep(srcPtr, copyPtr)
    Tcl_Obj *srcPtr;		/* Object with internal rep to copy. */
    Tcl_Obj *copyPtr;		/* Object with internal rep to set. */
{
    AbstractList *srcAbstractListRepPtr = Tcl_AbstractListRepPtr(srcPtr);
    AbstractList *copyAbstractListRepPtr;
    size_t repSize;
    /*
     * Allocate a new ArithSeries structure. 
     */

    repSize = srcAbstractListRepPtr->repSize; // example: sizeof(AbstractList) + sizeof(ArithSeries)
    copyAbstractListRepPtr = (AbstractList*) ckalloc(repSize);
    *copyAbstractListRepPtr = *srcAbstractListRepPtr;
    copyPtr->internalRep.twoPtrValue.ptr1 = copyAbstractListRepPtr;
    copyPtr->internalRep.twoPtrValue.ptr2 = (((char*)copyAbstractListRepPtr) + offsetof(AbstractList,abstractValue));
    copyAbstractListRepPtr->typeName = srcAbstractListRepPtr->typeName;
    copyPtr->typePtr = &tclAbstractListType;

    if (srcAbstractListRepPtr->dupRepProc) {
	srcAbstractListRepPtr->dupRepProc(srcPtr, copyPtr);
    }

}

/*
static void
DupAbstractListRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
    AbstractList *srcAbstractListRepPtr = Tcl_AbstractListRepPtr(srcPtr);
    AbstractList *copyAbstractListRepPtr = Tcl_AbstractListRepPtr(copyPtr);
    void *copyRepPtr = (void*)(((char*)copyAbstractListRepPtr) + offsetof(AbstractList,typeName));
    DupAbstractListInternalRep(srcPtr, copyPtr);

    copyAbstractListRepPtr->repSize = srcAbstractListRepPtr->repSize;
    srcAbstractListRepPtr->dupRepProc(srcPtr, copyPtr);
    copyPtr->internalRep.twoPtrValue.ptr2 = copyRepPtr;
}
*/

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfAbstractList --
 *
 *	Update the string representation for an abstractlist object.
 *	Note: This procedure does not invalidate an existing old string rep
 *	so storage will be lost if this has not already been done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's string is set to a valid string that results from the
 *	listlike-to-string conversion. This string will be empty if the
 *	AbstractList is empty.
 *
 * Notes:
 *      This simple approach is costly in that it forces a string rep for each
 *      element, which is then tossed.  Improving the performance here may
 *      require implementing a custom size-calculation function for each
 *      subtype of AbstractList.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfAbstractList(Tcl_Obj *abstractListObjPtr)
{
    AbstractList *abstractListRepPtr =
	    (AbstractList*) abstractListObjPtr->internalRep.twoPtrValue.ptr1;
    char *p, *str;
    Tcl_Obj *eleObj;
    Tcl_WideInt length = 0;
    int llen, slen, i;

    /*
     * Pass 1: estimate space.
     */
    llen = abstractListRepPtr->lengthProc(abstractListObjPtr);
    if (llen <= 0) {
	Tcl_InitStringRep(abstractListObjPtr, NULL, 0);
	return;
    }
    for (i = 0; i < llen; i++) {
	eleObj = abstractListRepPtr->indexProc(abstractListObjPtr, i);
	Tcl_GetStringFromObj(eleObj, &slen);
	length += slen + 1; /* one more for the space char */
	Tcl_DecrRefCount(eleObj);
    }

    /*
     * Pass 2: generate the string repr.
     */

    p = Tcl_InitStringRep(abstractListObjPtr, NULL, length);
    for (i = 0; i < llen; i++) {
	eleObj = abstractListRepPtr->indexProc(abstractListObjPtr, i);
	str = Tcl_GetStringFromObj(eleObj, &slen);
	strcpy(p, str);
	p[slen] = ' ';
	p += slen+1;
	Tcl_DecrRefCount(eleObj);
    }
    if (length > 0) abstractListObjPtr->bytes[length-1] = '\0';
    abstractListObjPtr->length = length-1;
}

/*
 *----------------------------------------------------------------------
 *
 * SetAbstractListFromAny --
 *
 * 	The Arithmetic Series object is just an way to optimize
 * 	Lists space complexity, so no one should try to convert
 * 	a string to an Arithmetic Series object.
 *
 * 	This function is here just to populate the Type structure.
 *
 * Results:
 *
 * 	The result is always TCL_ERROR. But see Side Effects.
 *
 * Side effects:
 *
 * 	Tcl Panic if called.
 *
 *----------------------------------------------------------------------
 */

static int
SetAbstractListFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    (void)interp;
    (void)objPtr;
    Tcl_Panic("SetAbstractListFromAny: should never be called");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclAbstractListObjCopy --
 *
 *	Makes a "pure AbstractList" copy of an AbstractList value. This
 *	provides for the C level a counterpart of the [lrange $list 0 end]
 *	command, while using internals details to be as efficient as possible.
 *
 * Results:
 *
 *	Normally returns a pointer to a new Tcl_Obj, that contains the same
 *	abstractList value as *abstractListPtr does. The returned Tcl_Obj has a
 *	refCount of zero. If *abstractListPtr does not hold an AbstractList,
 *	NULL is returned, and if interp is non-NULL, an error message is
 *	recorded there.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TclAbstractListObjCopy(
    Tcl_Interp *interp,		 /* Used to report errors if not NULL. */
    Tcl_Obj *abstractListObjPtr) /* List object for which an element array is
				  * to be returned. */
{
    Tcl_Obj *copyPtr;
    AbstractList *abstractListRepPtr;

    abstractListRepPtr = AbstractListGetInternalRep(abstractListObjPtr);
    if (NULL == abstractListRepPtr) {
	if (SetAbstractListFromAny(interp, abstractListObjPtr) != TCL_OK) {
	    /* We know this is going to panic, but it's the message we want */
	    return NULL;
	}
    }

    TclNewObj(copyPtr);
    TclInvalidateStringRep(copyPtr);
    DupAbstractListInternalRep(abstractListObjPtr, copyPtr);
    return copyPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AbstractListObjRange --
 *
 *	Makes a slice of an AbstractList value.
 *      *abstractListObjPtr must be known to be a valid AbstractList.
 *
 * Results:
 *	Returns a pointer to the sliced array.
 *      This may be a new object or the same object if not shared.
 *
 * Side effects:
 *
 *	?The possible conversion of the object referenced by
 *	abstractListObjPtr to a list object.?
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_AbstractListObjRange(
    Tcl_Obj *abstractListObjPtr, /* List object to take a range from. */
    Tcl_WideInt fromIdx,	 /* Index of first element to include. */
    Tcl_WideInt toIdx)		 /* Index of last element to include. */
{
    AbstractList *abstractListRepPtr = 
	AbstractListGetInternalRep(abstractListObjPtr);
    return abstractListRepPtr->sliceProc(abstractListObjPtr, fromIdx, toIdx);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AbstractListObjReverse --
 *
 *	Reverses the order of an AbstractList value.
 *      *abstractListObjPtr must be known to be a valid AbstractList.
 *
 * Results:
 *	Returns a pointer to the reversed array.
 *      This may be a new object or the same object if not shared.
 *
 * Side effects:
 *
 *	?The possible conversion of the object referenced by
 *	abstractListObjPtr to a list object.?
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_AbstractListObjReverse(
    Tcl_Obj *abstractListObjPtr) /* List object to take a range from. */
{
    AbstractList *abstractListRepPtr = 
	AbstractListGetInternalRep(abstractListObjPtr);
    return abstractListRepPtr->reverseProc(abstractListObjPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_AbstractListObjGetElements --
 *
 *	This function returns an (objc,objv) array of the elements in a list
 *	object.
 *
 * Results:
 *	The return value is normally TCL_OK; in this case *objcPtr is set to
 *	the count of list elements and *objvPtr is set to a pointer to an
 *	array of (*objcPtr) pointers to each list element. If listPtr does not
 *	refer to an Abstract List object and the object can not be converted
 *	to one, TCL_ERROR is returned and an error message will be left in the
 *	interpreter's result if interp is not NULL.
 *
 *	The objects referenced by the returned array should be treated as
 *	readonly and their ref counts are _not_ incremented; the caller must
 *	do that if it holds on to a reference. Furthermore, the pointer and
 *	length returned by this function may change as soon as any function is
 *	called on the list object; be careful about retaining the pointer in a
 *	local data structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AbstractListObjGetElements(
    Tcl_Interp *interp,		/* Used to report errors if not NULL. */
    Tcl_Obj *objPtr,		/* AbstractList object for which an element
				 * array is to be returned. */
    int *objcPtr,		/* Where to store the count of objects
				 * referenced by objv. */
    Tcl_Obj ***objvPtr)		/* Where to store the pointer to an array of
				 * pointers to the list's objects. */
{

    if (TclHasInternalRep(objPtr,&tclAbstractListType)) {
	AbstractList *abstractListRepPtr = 
	    AbstractListGetInternalRep(objPtr);
	Tcl_Obj **objv;
	int i, objc = abstractListRepPtr->lengthProc(objPtr);

	if (objc > 0) {
	    if (abstractListRepPtr->elements) {
		objv = abstractListRepPtr->elements;
	    } else {
		objv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*) * objc);
		if (objv == NULL) {
		    if (interp) {
			Tcl_SetObjResult(
			    interp,
			    Tcl_NewStringObj("max length of a Tcl list exceeded", -1));
			Tcl_SetErrorCode(interp, "TCL", "MEMORY", NULL);
		    }
		    return TCL_ERROR;
		}
		abstractListRepPtr->elements = objv;
		for (i = 0; i < objc; i++) {
		    objv[i] = abstractListRepPtr->indexProc(objPtr, i);
		    Tcl_IncrRefCount(objv[i]);
		}
	    }
	} else {
	    objv = NULL;
	}
	*objvPtr = objv;
	*objcPtr = objc;
    } else {
	if (interp != NULL) {
	    Tcl_SetObjResult(
		interp,
		Tcl_ObjPrintf("value is not an abstract list"));
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "UNKNOWN", NULL);
	}
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
Tcl_AbstractListSetProc(
    Tcl_Obj *objPtr,
    Tcl_AbstractListProcType ptype,
    void *proc)
{
    AbstractList* al_internalRep = AbstractListGetInternalRep(objPtr);

    if (al_internalRep == NULL) {
	return TCL_ERROR;
    }

    switch (ptype) {
    case TCL_ABSL_NEW:
	al_internalRep->newObjProc = (Tcl_ALNewObjProc *)proc;
	break;
    case TCL_ABSL_DUPREP:
	al_internalRep->dupRepProc = (Tcl_ALDupRepProc *)proc;
	break;
    case TCL_ABSL_LENGTH:
	al_internalRep->lengthProc = (Tcl_ALLengthProc *)proc;
	break;
    case TCL_ABSL_INDEX:
	al_internalRep->indexProc = (Tcl_ALIndexProc *)proc;
	break;
    case TCL_ABSL_SLICE:
	al_internalRep->sliceProc = (Tcl_ALSliceProc *)proc;
	break;
    case TCL_ABSL_REVERSE:
	al_internalRep->reverseProc = (Tcl_ALReverseProc *)proc;
	break;
    default:
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
