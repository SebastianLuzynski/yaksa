/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>

int yaksi_type_create_dup(yaksi_type_s * intype, yaksi_type_s ** newtype)
{
    int rc = YAKSA_SUCCESS;

    yaksu_atomic_incr(&intype->refcount);
    *newtype = intype;

    return rc;
}

int yaksa_type_create_dup(yaksa_type_t oldtype, yaksa_type_t * newtype)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksi_global.is_initialized);

    yaksi_type_s *intype;
    rc = yaksi_type_get(oldtype, &intype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    yaksi_type_s *outtype;
    rc = yaksi_type_create_dup(intype, &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    uint32_t id;
    rc = yaksi_type_handle_alloc(outtype, &id);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *newtype = (yaksa_type_t) id;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
