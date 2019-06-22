// Copyright 2019 Moritz Kiehn
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/// \file
/// \brief   ROOT-based storage of named tuples; requires named tuples library
/// \author  Moritz Kiehn <msmk@cern.ch>
/// \date    2019-04-00, Initial version

#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <TFile.h>
#include <TTree.h>

namespace dfe {

/// Write records in a ROOT TTree.
template<typename NamedTuple>
class RootNamedTupleWriter {
public:
  RootNamedTupleWriter() = delete;
  RootNamedTupleWriter(const RootNamedTupleWriter&) = delete;
  RootNamedTupleWriter(RootNamedTupleWriter&&) = delete;
  RootNamedTupleWriter& operator=(const RootNamedTupleWriter&) = delete;
  RootNamedTupleWriter& operator=(RootNamedTupleWriter&&) = delete;

  /// Create a file at the given path. Overwrites existing data.
  ///
  /// \param path       Path to the output file
  /// \param tree_name  Name of the output tree within the file
  RootNamedTupleWriter(const std::string& path, const std::string& tree_name);
  /// Create a tree in a ROOT directory. Overwrites existing data.
  ///
  /// \param dir        Output directory for the tree
  /// \param tree_name  Name of the output tree relative to the directory
  ///
  /// When the writer is created with an existing ROOT directory, the user
  /// is responsible for ensuring the underlying file is closed.
  RootNamedTupleWriter(TDirectory* dir, const std::string& tree_name);
  /// Write the tree and close the owned file.
  ~RootNamedTupleWriter();

  /// Append a record to the file.
  void append(const NamedTuple& record);

private:
  template<std::size_t... I>
  void setup_branches(std::index_sequence<I...>);

  TFile* m_file;
  TTree* m_tree;
  typename NamedTuple::Tuple m_data;
};

/// Read records from a ROOT TTree.
template<typename NamedTuple>
class RootNamedTupleReader {
public:
  RootNamedTupleReader() = delete;
  RootNamedTupleReader(const RootNamedTupleReader&) = delete;
  RootNamedTupleReader(RootNamedTupleReader&&) = delete;
  RootNamedTupleReader& operator=(const RootNamedTupleReader&) = delete;
  RootNamedTupleReader& operator=(RootNamedTupleReader&&) = delete;

  /// Open a file at the given path.
  ///
  /// \param path       Path to the input file
  /// \param tree_name  Name of the input tree within the file
  RootNamedTupleReader(const std::string& path, const std::string& tree_name);
  /// Open a tree from a ROOT directory.
  ///
  /// \param dir        Input directory for the tree
  /// \param tree_name  Name of the input tree relative to the directory
  ///
  /// When the reader is created with an existing ROOT directory, the user
  /// is responsible for ensuring the underlying file is closed.
  RootNamedTupleReader(TDirectory* dir, const std::string& tree_name);
  /// Write the tree and close the owned file.
  ~RootNamedTupleReader();

  /// Read the next record from the file.
  ///
  /// \returns true   if a record was successfully read
  /// \returns false  if no more records are available
  bool read(NamedTuple& record);

private:
  template<std::size_t... I>
  void setup_branches(std::index_sequence<I...>);

  TFile* m_file;
  TTree* m_tree;
  int64_t m_next;
  typename NamedTuple::Tuple m_data;
};

// implementation writer

template<typename NamedTuple>
inline RootNamedTupleWriter<NamedTuple>::RootNamedTupleWriter(
  const std::string& path, const std::string& tree_name)
  : m_file(new TFile(path.c_str(), "RECREATE"))
  , m_tree(new TTree(tree_name.c_str(), "", 99, m_file))
{
  if (not m_file) { throw std::runtime_error("Could not create file"); }
  if (not m_file->IsOpen()) { throw std::runtime_error("Could not open file"); }
  if (not m_tree) { throw std::runtime_error("Could not create tree"); }
  setup_branches(std::make_index_sequence<NamedTuple::N>());
}

template<typename NamedTuple>
inline RootNamedTupleWriter<NamedTuple>::RootNamedTupleWriter(
  TDirectory* dir, const std::string& tree_name)
  : m_file(nullptr) // no file since it is not owned by the writer
  , m_tree(new TTree(tree_name.c_str(), "", 99, dir))
{
  if (not dir) { throw std::runtime_error("Invalid output directory given"); }
  if (not m_tree) { throw std::runtime_error("Could not create tree"); }
  setup_branches(std::make_index_sequence<NamedTuple::N>());
}

namespace namedtuple_root_impl {
namespace {
template<typename T>
constexpr std::enable_if_t<false, T> kRootTypeCode;
template<>
constexpr char kRootTypeCode<bool> = 'O';
template<>
constexpr char kRootTypeCode<uint8_t> = 'b';
template<>
constexpr char kRootTypeCode<uint16_t> = 's';
template<>
constexpr char kRootTypeCode<uint32_t> = 'i';
template<>
constexpr char kRootTypeCode<uint64_t> = 'l';
template<>
constexpr char kRootTypeCode<ULong64_t> = 'l';
template<>
constexpr char kRootTypeCode<char> = 'B';
template<>
constexpr char kRootTypeCode<int8_t> = 'B';
template<>
constexpr char kRootTypeCode<int16_t> = 'S';
template<>
constexpr char kRootTypeCode<int32_t> = 'I';
template<>
constexpr char kRootTypeCode<int64_t> = 'L';
template<>
constexpr char kRootTypeCode<Long64_t> = 'L';
template<>
constexpr char kRootTypeCode<float> = 'F';
template<>
constexpr char kRootTypeCode<double> = 'D';
} // namespace
} // namespace namedtuple_root_impl

template<typename NamedTuple>
template<std::size_t... I>
inline void
RootNamedTupleWriter<NamedTuple>::setup_branches(std::index_sequence<I...>)
{
  static_assert(sizeof...(I) == NamedTuple::N, "Something is very wrong");

  // construct leaf names w/ type info
  std::array<std::string, sizeof...(I)> names = NamedTuple::names();
  std::array<std::string, sizeof...(I)> leafs = {
    (names[I] + '/' +
     namedtuple_root_impl::kRootTypeCode<
       std::tuple_element_t<I, typename NamedTuple::Tuple>>)...};
  // construct branches
  // NOTE 2019-05-13 msmk:
  // the documentation suggests that ROOT can figure out the branch types on
  // its own, but doing so seems to break for {u}int64_t. do it manually for
  // now.
  (void)std::array<TBranch*, sizeof...(I)>{m_tree->Branch(
    names[I].c_str(), &std::get<I>(m_data), leafs[I].c_str())...};
}

template<typename NamedTuple>
inline RootNamedTupleWriter<NamedTuple>::~RootNamedTupleWriter()
{
  // alway overwrite old data
  if (m_tree) { m_tree->Write(nullptr, TObject::kOverwrite); }
  // writer owns the file
  if (m_file) {
    m_file->Close();
    delete m_file;
  }
}

template<typename NamedTuple>
inline void
RootNamedTupleWriter<NamedTuple>::append(const NamedTuple& record)
{
  m_data = record.to_tuple();
  if (m_tree->Fill() == -1) {
    throw std::runtime_error("Could not fill an entry");
  }
}

// implementation reader

template<typename NamedTuple>
inline RootNamedTupleReader<NamedTuple>::RootNamedTupleReader(
  const std::string& path, const std::string& tree_name)
  : m_file(new TFile(path.c_str(), "READ"))
  , m_tree(nullptr)
  , m_next(0)
{
  if (not m_file) { throw std::runtime_error("Could not open file"); }
  if (not m_file->IsOpen()) { throw std::runtime_error("Could not open file"); }
  m_tree = static_cast<TTree*>(m_file->Get(tree_name.c_str()));
  if (not m_tree) { throw std::runtime_error("Could not read tree"); }
  setup_branches(std::make_index_sequence<NamedTuple::N>());
}

template<typename NamedTuple>
inline RootNamedTupleReader<NamedTuple>::RootNamedTupleReader(
  TDirectory* dir, const std::string& tree_name)
  : m_file(nullptr) // no file since it is not owned by the writer
  , m_tree(nullptr)
  , m_next(0)
{
  if (not dir) { throw std::runtime_error("Invalid input directory given"); }
  m_tree = static_cast<TTree*>(dir->Get(tree_name.c_str()));
  if (not m_tree) { throw std::runtime_error("Could not read tree"); }
  setup_branches(std::make_index_sequence<NamedTuple::N>());
}

namespace namedtuple_root_impl {
namespace {
// WARNING this is a hack to get around inconsistent ROOT types for 8bit chars
// and 64bit intengers compared to the stdint types.
__attribute__((unused)) inline ULong64_t*
get_address(uint64_t& x)
{
  static_assert(
    sizeof(ULong64_t) == sizeof(uint64_t), "Inconsistent type sizes");
  return reinterpret_cast<ULong64_t*>(&x);
}
__attribute__((unused)) inline char*
get_address(int8_t& x)
{
  static_assert(sizeof(char) == sizeof(int8_t), "Inconsistent type sizes");
  return reinterpret_cast<char*>(&x);
}
__attribute__((unused)) inline Long64_t*
get_address(int64_t& x)
{
  static_assert(sizeof(Long64_t) == sizeof(int64_t), "Inconsistent type sizes");
  return reinterpret_cast<Long64_t*>(&x);
}
template<typename T>
inline T*
get_address(T& x)
{
  return &x;
}
} // namespace
} // namespace namedtuple_root_impl

template<typename NamedTuple>
template<std::size_t... I>
inline void
RootNamedTupleReader<NamedTuple>::setup_branches(std::index_sequence<I...>)
{
  static_assert(sizeof...(I) == NamedTuple::N, "Something is very wrong");

  // construct leaf names w/ type info
  std::array<std::string, sizeof...(I)> names = NamedTuple::names();
  // construct branches
  (void)std::array<Int_t, sizeof...(I)>{m_tree->SetBranchAddress(
    names[I].c_str(),
    namedtuple_root_impl::get_address(std::get<I>(m_data)))...};
}

template<typename NamedTuple>
inline RootNamedTupleReader<NamedTuple>::~RootNamedTupleReader()
{
  // reader owns the file
  if (m_file) {
    m_file->Close();
    delete m_file;
  }
}

template<typename NamedTuple>
inline bool
RootNamedTupleReader<NamedTuple>::read(NamedTuple& record)
{
  auto ret = m_tree->GetEntry(m_next);
  // i/o error occured
  if (ret < 0) { throw std::runtime_error("Could not read entry"); }
  // the entry does not exist, probably end-of-file reached
  if (ret == 0) { return false; }
  // GetEntry(...) has already filled the local buffer
  record = m_data;
  m_next += 1;
  return true;
}

} // namespace dfe
