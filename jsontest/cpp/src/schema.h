#pragma once

#include <arrow/api.h>

#include <any>
#include <functional>
#include <list>
#include <utility>

#include "status.h"

struct NodeRef {
  NodeRef(std::list<std::string> path, std::function<arrow::Status(std::any)> append)
      : path(std::move(path)), Append(std::move(append)) {}
  std::list<std::string> path;
  std::function<arrow::Status(std::any)> Append;
};

class DOMPointerGenerator : public arrow::TypeVisitor {
 public:
  auto Analyze(const arrow::Schema& schema) -> Status;
  auto VisitField(const arrow::Field& field) -> Status;
  auto VisitType(const arrow::DataType& type) -> Status;
  // Non-nested types:
  auto Visit(const arrow::UInt64Type& type) -> arrow::Status override;
  // Nested types:
  auto Visit(const arrow::ListType& type) -> arrow::Status override;
  auto Visit(const arrow::StructType& type) -> arrow::Status override;

  [[nodiscard]] auto ToString() const -> std::string;
  [[nodiscard]] auto nodes() const -> std::list<NodeRef> { return nodes_; }

 protected:
  // The path currently being visited in the expected DOM tree.
  std::list<std::string> path_;

  // Flattened paths in the dom tree and associated builders.
  std::list<NodeRef> nodes_;

  std::vector<std::shared_ptr<arrow::ArrayBuilder>> builders;
};