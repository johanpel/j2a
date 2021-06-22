#include "schema.h"

#include <any>
#include <functional>
#include <iomanip>
#include <sstream>

#include "status.h"

auto DOMPointerGenerator::Analyze(const arrow::Schema& schema) -> Status {
  for (const auto& field : schema.fields()) {
    VisitField(*field);
  }
  return Status::OK;
}

auto DOMPointerGenerator::VisitField(const arrow::Field& field) -> Status {
  VisitType(*field.type());
  return Status::OK;
}

auto DOMPointerGenerator::VisitType(const arrow::DataType& type) -> Status {
  ARROW_ROE(type.Accept(this));
  return Status::OK;
}

auto AppendInt(arrow::UInt64Builder* bld, uint64_t val) -> arrow::Status {
  return arrow::Status::OK();
}

auto DOMPointerGenerator::Visit(const arrow::UInt64Type& type) -> arrow::Status {
  auto builder = std::make_shared<arrow::UInt64Builder>();
  builders.push_back(builder);

  auto nr = NodeRef(path_, std::bind(AppendInt, builder.get(), std::placeholders::_1));

  path_.clear();
  return arrow::Status::OK();
}

auto DOMPointerGenerator::Visit(const arrow::ListType& type) -> arrow::Status {
  // Push back the node for the list builder.
  // We can't construct the list builder yet, because we need the values builder first.
  auto& this_node_ref = nodes_.emplace_back(path_);

  // Get the iterator to the current node.
  auto iter = nodes_.end()--;

  // Walk down the tree.
  VisitField(*type.value_field());

  // Move to the child node that was appended.
  // Construct the list builder, taking the child node builder.
  this_node_ref.builder = std::make_shared<arrow::ListBuilder>(
      arrow::default_memory_pool(), (++iter)->builder);

  return arrow::Status::OK();
}

auto DOMPointerGenerator::Visit(const arrow::StructType& type) -> arrow::Status {
  // Push back the node for the struct builder.
  // We can't construct the struct builder yet, because we need the values builders first.
  auto& this_node_ref = nodes_.emplace_back(join(path_));
  // Get the iterator to the current node.
  auto iter = --nodes_.end();
  // Make a copy of the current path, so we can restore it after visiting every field.
  auto this_path = path_;

  std::vector<std::shared_ptr<arrow::ArrayBuilder>> child_builders;

  for (const auto& field : type.fields()) {
    // Visit the child field.
    VisitField(*field);
    // Move to the child node.
    // Push back a copy of the pointer to the child builder.
    child_builders.push_back((++iter)->builder);
    // Set the iterator to the last node.
    iter = --nodes_.end();
    // Restore the current path.
    path_ = this_path;
  }

  // Create the struct builder and place it on the current node.
  this_node_ref.builder = std::make_shared<arrow::StructBuilder>(
      arrow::struct_(type.fields()), arrow::default_memory_pool(), child_builders);

  return arrow::Status::OK();
}

auto DOMPointerGenerator::ToString() const -> std::string {
  std::ostringstream ss;
  size_t longest = 0;
  for (const auto& node : nodes_) {
    if (node.name.length() > longest) {
      longest = node.name.length();
    }
  }
  for (const auto& node : nodes_) {
    ss << std::left << std::setw(longest) << node.name << " : "
       << (node.builder == nullptr ? "nullptr" : node.builder->type()->name())
       << std::endl;
  }
  return ss.str();
}