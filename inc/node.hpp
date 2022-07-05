//tracks the state of a node within a model
//a node is a subset of a model consisting of its own transform and primitives that behave under it.

#pragma once

#include "data_structures.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace model {
	class Node {
	private:
		glm::mat4 matrix;
		std::vector<std::unique_ptr<Node>> children;
		Node* parent_node; //the parent of the parent will manage the lifetime

	public:
		Node(const glm::mat4& matrix, Node* parent_node = nullptr);
		Node(Node* parent_node = nullptr);
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;

		void add_matrix(const glm::mat4& matrix);
		void add_node(std::unique_ptr<Node> node);
		void add_parent(Node* parent_node);

		glm::mat4 get_transform() ;
		Node* get_parent() ;
};
}