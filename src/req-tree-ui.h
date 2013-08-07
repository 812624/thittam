/* Copyright (C) 2012,2013 HiPro IT Solutions Private Limited,
 * Chennai. All rights reserved.
 *
 * This program and the accompanying materials are made available
 * under the terms described in the LICENSE file which accompanies
 * this distribution. If the LICENSE file was not attached to this
 * distribution or for further clarifications, please contact
 * legal@hipro.co.in. */

#ifndef HIPRO_THITTAM__a79a60a8_fe59_11e2_911b_68a3c42125fd
#define HIPRO_THITTAM__a79a60a8_fe59_11e2_911b_68a3c42125fd

#include <memory>
#include <string>

#include <boost/signals2.hpp>

#include "requirement.h"
#include "req-tree.h"

class ReqTreeUI
{
public:
  typedef std::shared_ptr<ReqTreeUI> ptr_t;

  typedef boost::signals2::signal <void (Requirement::ptr_t req)> signal_edit_t;
public:
  virtual void load (ReqTree::ptr_t req_tree) = 0;

  virtual ~ReqTreeUI () {}
};
#endif // HIPRO_THITTAM__a79a60a8_fe59_11e2_911b_68a3c42125fd

/*
  Local Variables:
  mode: c++
  indent-tabs-mode: nil
  tab-width: 4
  c-file-style: "gnu"
  End:
*/