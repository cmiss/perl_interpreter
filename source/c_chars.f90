MODULE C_CHARS

!! Provides mechanisms for converting between C strings and Fortran characters
!! Written by K.A.Tomlinson 6/4/00

USE KINDS

IMPLICIT NONE
 
PRIVATE
 
!-----------------------------------------------------------------------------!
! By default all entities declared or defined in this module are private to   !
! the module. Only those entities declared explicitly as being public are     !
! accessible to programs using the module. In particular, the procedures and  !
! operators defined herein are made accessible via their generic identifiers  !
! only; their specific names are private.                                     !
!-----------------------------------------------------------------------------!

!#### Type: C_CHAR_TYPE
!###  Description:
!###    Equivalent to char in the C language.
!###    The assignment operator is defined for conversions between CHARACTER
!###    and C_CHAR(:) for conversions between fortran CHARACTER and C strings.
!###    C_CHAR_NULL is defined as the NULL terminator char.
!###  See-Also: C_STRING,CHARACTER,STRLEN

TYPE C_CHAR_TYPE
  PRIVATE
  SEQUENCE
  CHARACTER :: char
ENDTYPE C_CHAR_TYPE

TYPE(C_CHAR_TYPE), PARAMETER :: C_CHAR_NULL = C_CHAR_TYPE(CHAR(0))

!----- GENERIC PROCEDURE INTERFACE DEFINITIONS -------------------------------!
 
!----- C String length interface ---------------------------------------------!

! This is named STRLEN instead of LEN because the string must be defined
! before this can be evaluated.
INTERFACE STRLEN
  !#### Function: STRLEN(C_CHAR_TYPE(*))
  !###  Description:
  !###    Returns the length of C string.
  MODULE PROCEDURE strlen_c
  !#### Function: STRLEN(C_CHAR_TYPE(*),INTEGER)
  !###  Description:
  !###    Searches up to a specified number of characters for the end of C
  !###    string.  Returns the length of the string if the end was found, or
  !###    the number of characters searched.
  MODULE PROCEDURE strlen_c_lim 
ENDINTERFACE
 
!----- Conversion procedure interfaces ---------------------------------------!
INTERFACE CHARACTER
  !#### Function: CHARACTER(C_CHAR_TYPE(:))
  !###  Description:
  !###    Returns an automatic-length Fortran CHARACTER with characters
  !###    from the supplied array of C chars.
  MODULE PROCEDURE c_to_f   ! C string to Fortran character
  !#### Function: CHARACTER(C_CHAR_TYPE(*),INTEGER)
  !###  Description:
  !###    Returns a Fortran CHARACTER of specified length with characters from
  !###    the supplied C string, truncated or padded with blanks as
  !###    necessary.
  MODULE PROCEDURE c_to_fix_f
END INTERFACE
INTERFACE C_STRING
  !#### Function: C_STRING(CHARACTER)
  !###  Description:
  !###    Returns a C string equivalent to the supplied Fortran CHARACTER
  MODULE PROCEDURE f_to_c
END INTERFACE
 
!----- ASSIGNMENT interfaces -------------------------------------------------!
INTERFACE ASSIGNMENT(=)
  MODULE PROCEDURE f_ass_c, &   ! Fortran character = C string
                   c_ass_f      ! C string          = Fortran character
END INTERFACE

!----- specification of publically accessible entities -----------------------!
PUBLIC :: C_CHAR_TYPE,C_CHAR_NULL,C_STRING,CHARACTER,STRLEN,ASSIGNMENT(=)
 
CONTAINS

!----- LEN Procedures ---------------------------------------------------------!
  PURE FUNCTION strlen_c(string)
    TYPE(C_CHAR_TYPE),INTENT(IN) :: string(0:*)
    INTEGER(INTG)                :: strlen_c
    ! returns the length of the C string argument
    INTEGER(INTG) :: i
    i = 0
    DO
      IF(string(i)%char==C_CHAR_NULL%char) EXIT
      i = i + 1
    ENDDO
    strlen_c = i
  ENDFUNCTION strlen_c
 
  FUNCTION strlen_c_lim(string,lim)
    TYPE(C_CHAR_TYPE),INTENT(IN) :: string(0:*)
    INTEGER(INTG),INTENT(IN)     :: lim
    INTEGER(INTG) :: strlen_c_lim
    ! returns the length of the C string argument
    INTEGER(INTG) :: i
    i = 0
    DO
      IF(i==lim) EXIT
      IF(string(i)%char==C_CHAR_NULL%char) EXIT
      i = i + 1
    ENDDO
    strlen_c_lim = i
  ENDFUNCTION strlen_c_lim

!----- Conversion Procedures ------------------------------------------------!
  FUNCTION c_to_f(string)
    TYPE(C_CHAR_TYPE),INTENT(IN) :: string(:)
    CHARACTER(LEN=SIZE(string)) :: c_to_f
    ! Returns a Fortran character containing the chars of a C string.
    ! It would be nice if string could be assumed size and the length obtained
    ! from STRLEN function but this didn't work with SGIs 7.3 compiler.
    ! The size of the supplied string therefore must be such that the null
    ! terminator is not included.
!!    CHARACTER(LEN=STRLEN(string)) :: c_to_f
    ! Actually, this current format allows us to do extraction of substrings
    ! but it would be necessary to check that a null termination does not occur
    ! earlier in the string.
    
!!    ! and allows pointers to strings that haven't been defined.
!!    ! KAT 4/3/00: I can't find anything in the fortran standard saying that
!!    ! lengths of strings must agree in a pointer assignment.
!!    c_to_f => string(0)%char
    c_to_f = TRANSFER(string,c_to_f)
  ENDFUNCTION c_to_f

  FUNCTION c_to_fix_f(string,length)
    TYPE(C_CHAR_TYPE),INTENT(IN) :: string(*)
    INTEGER(INTG),INTENT(IN)     :: length
    CHARACTER(LEN=length) :: c_to_fix_f
    ! Returns the Fortran character of fixed length, length, containing the
    ! characters of a C string either padded with blanks or truncated on the
    ! right to fit.  The C string may include a null and following
    ! contents are ignored.
!!    c_to_fix_f = CHARACTER(string(:LEN(string,length)))
    INTEGER(INTG) :: string_len
    string_len = STRLEN(string,length)
    c_to_fix_f = TRANSFER(string(:string_len),c_to_fix_f(:string_len))
  ENDFUNCTION c_to_fix_f

  FUNCTION f_to_c(char)
    CHARACTER(LEN=*),INTENT(IN) :: char
    TYPE(C_CHAR_TYPE) :: f_to_c(LEN(char)+1)
    ! Returns a C string containing the chars of a Fortran character.
!!    f_to_c(:LEN(char)) = TRANSFER(char,f_to_c,LEN(char))
!!    f_to_c(LEN(char)+1) = C_CHAR_NULL
    f_to_c = (/ TRANSFER(char,f_to_c,LEN(char)), C_CHAR_NULL /)
  ENDFUNCTION f_to_c

!----- ASSIGNMENT Procedures -------------------------------------------------!

  SUBROUTINE c_ass_f(destination,source)
    TYPE(C_CHAR_TYPE),INTENT(OUT) :: destination(:)
    CHARACTER(LEN=*),INTENT(IN)   :: source
    ! Assign a Fortran character source to a C string destination
    ! The fortran character is truncated if necessary and a null always
    ! completes the C string.
!!    CHARACTER(LEN=MIN(UBOUND(destination,1),LEN(source))),POINTER :: char_ptr
!!    char_ptr => CHARACTER(destination(LEN(char_ptr)))
!!    char_ptr = source !copy characters to destination
!!    destination(LEN(char_ptr)) = C_CHAR_NULL
    INTEGER(INTG) :: string_len
    string_len=MIN(SIZE(destination)-1,LEN(source))
!!    destination(:string_len+1) = C_CHAR(source(:string_len))
!!    destination(:string_len) = TRANSFER(source,destination,string_len)
!!    destination(string_len+1) = C_CHAR_NULL
    destination(:string_len+1) = &
      & (/ TRANSFER(source,destination,string_len), C_CHAR_NULL /)
  ENDSUBROUTINE c_ass_f

  SUBROUTINE f_ass_c(destination,source)
    CHARACTER(LEN=*),INTENT(OUT) :: destination
    TYPE(C_CHAR_TYPE),INTENT(IN) :: source(*)
    ! Assign a C string to a Fortran character variable.
    ! If the string is longer than the character truncate the string on the right
    ! If the string is shorter the character is blank padded on the right
!!    destination = CHARACTER(source,LEN(destination))
    INTEGER(INTG) :: string_len
    string_len = STRLEN(source,LEN(destination))
    destination = TRANSFER(source(:string_len),destination(:string_len))
  ENDSUBROUTINE f_ass_c

ENDMODULE C_CHARS
