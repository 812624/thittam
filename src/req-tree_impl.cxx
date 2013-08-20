/* Copyright (C) 2012,2013 HiPro IT Solutions Private Limited,
 * Chennai. All rights reserved.
 *
 * This program and the accompanying materials are made available
 * under the terms described in the LICENSE file which accompanies
 * this distribution. If the LICENSE file was not attached to this
 * distribution or for further clarifications, please contact
 * legal@hipro.co.in. */

#include "req-tree_impl.h"

ReqTreeImpl::ReqTreeImpl (HLogPtr logger,
                          std::shared_ptr<RequirementFactory> req_factory)
  : m_logger (logger),
    m_req_factory (req_factory)
{
  m_last_id = 0;
  m_dirty = false;
  m_root = m_req_factory->create ("root", "root", "root");
}

std::string
ReqTreeImpl::add_child (std::shared_ptr<Requirement> parent,
                        std::shared_ptr<Requirement> child,
                        bool duplicate)
{
  std::shared_ptr<Requirement> real_child (child);
  if (duplicate == true)
    {
      real_child = duplicate_tree (child);
    }
  else
    {
      /* Here we assume that the child will not have children */
      real_child->set_id (next_id ());
    }
  add_to_req_id_map (real_child);

  std::shared_ptr<Requirement> real_parent (parent);
  if (!real_parent)
    {
      real_parent = m_root;
    }

  auto children = real_parent->children ();
  child->set_parent (real_parent);
  children->push_back (child);
  set_dirty_noisily ();
  return real_child->id ();
}

std::string
ReqTreeImpl::add_sibling (std::shared_ptr<Requirement> sibling,
                          std::shared_ptr<Requirement> new_req,
                          bool duplicate)
{
  auto parent = sibling->parent ();
  std::shared_ptr<Requirement> real_child (new_req);
  if (duplicate == true)
    {
      real_child = duplicate_tree (new_req);
    }
  else
    {
      /* Here we assume that the child will not have children */
      real_child->set_id (next_id ());
    }
  add_to_req_id_map (real_child);

  auto children = parent->children ();
  auto pos = std::find (children->begin (), children->end (), sibling);
  ASSERT ((pos != children->end ()), "sibling=%s not a child of %s",
          sibling->id ().c_str (), parent->id ().c_str ());
  pos ++;
  real_child->set_parent (parent);
  children->insert (pos, real_child);
  set_dirty_noisily ();
  return real_child->id ();
}

void
ReqTreeImpl::detach (std::shared_ptr<Requirement> node)
{
  auto parent = node->parent ();
  auto children = parent->children ();
  auto pos = std::find (children->begin (), children->end (), node);
  ASSERT ((pos != children->end ()), "node=%s is not a child of %s",
          node->id ().c_str (), parent->id ().c_str ());
  children->erase (pos);
  remove_from_req_id_map (node);
}

bool
ReqTreeImpl::is_first_child (std::shared_ptr<const Requirement> req) const
{
  auto parent = req->parent ();
  auto children = parent->children ();
  return (children->front () == req);
}

bool
ReqTreeImpl::is_last_child (std::shared_ptr<const Requirement> req) const
{
  auto parent = req->parent ();
  auto children = parent->children ();
  return (children->back () == req);
}

bool
ReqTreeImpl::is_top_level (std::shared_ptr<const Requirement> req) const
{
  auto parent = req->parent ();
  return (parent->id () == "root");
}

void
ReqTreeImpl::up_child (std::shared_ptr<Requirement> node)
{
  auto parent = node->parent ();
  auto children = parent->children ();
  auto pos = std::find (children->begin (), children->end (), node);
  ASSERT ((pos != children->end ()), "node=%s is not a child of %s",
          node->id ().c_str (), parent->id ().c_str ());
  ASSERT ((pos != children->begin ()),
          "cannot up-child. node=%s is first child", node->id ().c_str ());

  *(pos) = *(pos - 1);
  *(pos - 1) = node;
  set_dirty_noisily ();
}

void
ReqTreeImpl::down_child (std::shared_ptr<Requirement> node)
{
  auto parent = node->parent ();
  auto children = parent->children ();
  auto pos = std::find (children->begin (), children->end (), node);
  ASSERT ((pos != children->end ()), "node=%s is not a child of %s",
          node->id ().c_str (), parent->id ().c_str ());
  ASSERT ((*pos != children->back ()),
          "cannot down-child. node=%s is last child", node->id ().c_str ());

  *(pos) = *(pos + 1);
  *(pos + 1) = node;
  set_dirty_noisily ();
}

void
ReqTreeImpl::up_level (std::shared_ptr<Requirement> node)
{
  auto parent = node->parent ();
  auto grand_parent = parent->parent ();
  auto parent_children = parent->children ();
  auto grand_parent_children = grand_parent->children ();

  auto node_pos = std::find
    (parent_children->begin (), parent_children->end (), node);
  ASSERT ((node_pos != parent_children->end ()), "node=%s is not a child of %s",
          node->id ().c_str (), parent->id ().c_str ());

  auto parent_pos = std::find
    (grand_parent_children->begin (), grand_parent_children->end (), parent);
  ASSERT ((parent_pos != grand_parent_children->end ()),
          "node=%s is not a child of %s",
          node->id ().c_str (), parent->id ().c_str ());

  parent_children->erase (node_pos);
  parent_pos ++;
  node->set_parent (grand_parent);
  grand_parent_children->insert (parent_pos, node);
  set_dirty_noisily ();
}

void
ReqTreeImpl::down_level (std::shared_ptr<Requirement> node)
{
  auto parent = node->parent ();
  auto parent_children = parent->children ();

  auto node_pos = std::find
    (parent_children->begin (), parent_children->end (), node);
  ASSERT ((node_pos != parent_children->end ()), "node=%s is not a child of %s",
          node->id ().c_str (), parent->id ().c_str ());
  ASSERT ((node_pos != parent_children->begin ()),
          "node=%s is the first child. cannot down-level",
          node->id ().c_str ());

  auto sibling = *(node_pos - 1);
  auto sibling_children = sibling->children ();

  parent_children->erase (node_pos);
  node->set_parent (sibling);
  sibling_children->push_back (node);
  set_dirty_noisily ();
}

size_t
ReqTreeImpl::height (void) const
{
  return (compute_height (m_root) - 1);
}

std::shared_ptr<Requirement>
ReqTreeImpl::get (const std::string & reqid) const
{
  return m_req_id_map.at (reqid);
}

size_t
ReqTreeImpl::compute_height (std::shared_ptr<const Requirement> node) const
{
  size_t max_child_height = 0;
  auto end = node->children_cend ();
  for (auto it = node->children_cbegin (); it != end; ++it)
    {
      auto child = *it;
      size_t child_height = compute_height (child);
      if (child_height > max_child_height)
        {
          max_child_height = child_height;
        }
    }
  return (max_child_height + 1);
}

std::string
ReqTreeImpl::next_id (void)
{
  std::ostringstream os;
  m_last_id += 1;
  os << "r" << m_last_id;
  return os.str ();
}

std::shared_ptr<Requirement>
ReqTreeImpl::duplicate_tree (std::shared_ptr<const Requirement> node)
{
  auto new_node = m_req_factory->create
    (next_id (), node->title (), node->description ());
  auto children = new_node->children ();
  auto end = node->children_cend ();
  for (auto it = node->children_cbegin (); it != end; ++it)
    {
      auto child = *it;
      auto new_child = duplicate_tree (child);
      children->push_back (new_child);
    }
  return new_node;
}

void
ReqTreeImpl::add_to_req_id_map (std::shared_ptr<Requirement> req)
{
  m_req_id_map[req->id ()] = req;
  auto end = req->children_cend ();
  for (auto it = req->children_cbegin (); it != end; ++it)
    {
      auto child = *it;
      add_to_req_id_map (child);
    }
}

void
ReqTreeImpl::remove_from_req_id_map (std::shared_ptr<Requirement> req)
{
  m_req_id_map.erase (req->id ());
  auto end = req->children_cend ();
  for (auto it = req->children_cbegin (); it != end; ++it)
    {
      auto child = *it;
      remove_from_req_id_map (child);
    }
}


void
ReqTreeImpl::set_dirty_noisily (void)
{
  m_dirty = true;
  m_signal_tree_dirty ();
}

void
ReqTreeImpl::load (const std::string & data)
{
  ASSERT ((false), "not implemented");
}

void
ReqTreeImpl::load (const std::ifstream & file)
{
  ASSERT ((false), "not implemented");
}

std::string
ReqTreeImpl::serialize (void)
{
  ASSERT ((false), "not implemented");
  return "";
}

/*
  Local Variables:
  mode: c++
  indent-tabs-mode: nil
  End:
*/
