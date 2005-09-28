/*
 * $Id$
 *
 * Code for Optional Exception Handling through UseExceptions(),
 * DontUseExceptions()
 *
 * It uses CPLSetErrorHandler to provide a custom function
 * which notifies the bindings of errors. 
 *
 * This is not thread safe.
 *
 * $Log$
 * Revision 1.3  2005/09/28 18:24:36  kruland
 * Removed global flag bExceptionHappened.  Create a custom error handler which
 * writes messages for CE_Fatal errors.
 *
 * Revision 1.2  2005/09/18 07:36:18  cfis
 * Only raise exceptions on failures or fatal errors.  The previous code rose exceptions on debug messages, warning messages and when nothing at all happened.
 *
 * Revision 1.1  2005/09/13 03:04:27  kruland
 * Pull the exception generation mechanism out of gdal_python.i so it could
 * be used by other bindings.
 *
 *
 */

%{
int bUseExceptions=0;

void VeryQuiteErrorHandler(CPLErr eclass, int code, const char *msg ) {
  /* If the error class is CE_Fatal, we want to have a message issued
     because the CPL support code does an abort() before any exception
     can be generated */
  if (eclass == CE_Fatal ) {
    CPLDefaultErrorHandler(eclass, code, msg );
  }
}
%}

%inline %{
void UseExceptions() {
  bUseExceptions = 1;
  CPLSetErrorHandler( (CPLErrorHandler) VeryQuiteErrorHandler );
}

void DontUseExceptions() {
  bUseExceptions = 0;
  CPLSetErrorHandler( CPLDefaultErrorHandler );
}
%}

%include exception.i

%exception {
  {
    CPLErrorReset();
    $action
    if ( bUseExceptions ) {
      CPLErr eclass = CPLGetLastErrorType();
      if ( eclass == CE_Failure || eclass == CE_Fatal ) {
        SWIG_exception( SWIG_RuntimeError, CPLGetLastErrorMsg() );
      }
    }
  }
}
