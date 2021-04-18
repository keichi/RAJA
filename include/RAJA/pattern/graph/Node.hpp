/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file containing the core components of RAJA::graph::Node
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_pattern_graph_Node_HPP
#define RAJA_pattern_graph_Node_HPP

#include "RAJA/config.hpp"

#include <utility>
#include <vector>

namespace RAJA
{

namespace expt
{

namespace graph
{

template < typename policy >
struct DAG;

namespace detail {

struct NodeArgs
{ };

template < typename node_type, typename node_args >
RAJA_INLINE node_type*
make_Node(node_args&& arg);

}

struct Node
{
  RAJA_INLINE
  Node() = default;

  virtual void exec() = 0;

  virtual ~Node() = default;

  template < typename node_type >
  concepts::enable_if_t<node_type&,
                        std::is_base_of<Node, node_type>>
  operator>>(node_type& rhs)
  {
    return *add_child(&rhs);
  }

  template < typename node_args>
  auto operator>>(node_args&& rhs)
    -> concepts::enable_if_t<decltype(*std::forward<node_args>(rhs).toNode()),
                             std::is_base_of<detail::NodeArgs, camp::decay<node_args>>>
  {
    return *add_child(std::forward<node_args>(rhs).toNode());
  }

private:
  template < typename >
  friend struct DAG;

  template < typename Enter_Func, typename Exit_Func >
  static void forward_traverse(Node* node, Enter_Func&& enter_func, Exit_Func&& exit_func);

  int m_parent_count = 0;
  int m_count = 0;
  std::vector<Node*> m_children;

  template < typename node_type >
  concepts::enable_if_t<node_type*, std::is_base_of<Node, node_type>>
  add_child(node_type* node)
  {
    m_children.emplace_back(node);
    node->m_parent_count += 1;
    return node;
  }
};

template < typename Enter_Func, typename Exit_Func >
void Node::forward_traverse(Node* node, Enter_Func&& enter_func, Exit_Func&& exit_func)
{
  if (++node->m_count == node->m_parent_count) {
    node->m_count = 0;
    std::forward<Enter_Func>(enter_func)(node);
    for (Node* child : node->m_children)
    {
      forward_traverse(child, std::forward<Enter_Func>(enter_func), std::forward<Exit_Func>(exit_func));
    }
    std::forward<Exit_Func>(exit_func)(node);
  }
}

}  // namespace graph

}  // namespace expt

}  // namespace RAJA

#endif
