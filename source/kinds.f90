!#### Module: KINDS
!###  Description:
!###    This module contains kind definitions.
!###    $Id$

MODULE KINDS

  IMPLICIT NONE

  !This is intended for dummy arguments of unknown type
  TYPE VOID_TYPE
    PRIVATE
    INTEGER :: DATA(0)
  END TYPE VOID_TYPE
    
  !INTEGER kind parameters
  INTEGER, PARAMETER :: INTG = KIND(0)
  INTEGER, PARAMETER :: DBLP = KIND(0d0)

END MODULE KINDS
