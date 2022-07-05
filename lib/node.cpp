#include "node.hpp"

using namespace model;

Node::Node(const glm::mat4& matrix, Node* parent_node) {
	Node::matrix = matrix;
	Node::parent_node = parent_node;
}

Node::Node(Node* parent_node) {
	Node::parent_node = parent_node;
}

void Node::add_parent(Node* parent_node) {
	Node::parent_node = parent_node;
}

void Node::add_matrix(const glm::mat4& matrix) { 
	Node::matrix = matrix; 
}

void Node::add_node(std::unique_ptr<Node> node) {
	children.push_back(std::move(node));
}

Node* Node::get_parent() {
	return parent_node;
}

glm::mat4 Node::get_transform() {
	return matrix;
}