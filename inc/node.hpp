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
		std::vector<Node> children; //owns actual object, means if not careful, excessive copying will occur.
		Node* parent_node; //the parent of the parent will manage the lifetime

	public:
		Node(const glm::mat4& matrix, Node* parent_node = nullptr);
		Node(Node* parent_node = nullptr);

		void add_matrix(const glm::mat4& matrix);
		void add_child(Node node);
		void add_parent(Node* parent_node);

		glm::mat4 get_transform() ;
		Node* get_parent() ;
};
}