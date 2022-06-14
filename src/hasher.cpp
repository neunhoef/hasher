// hasher.cpp - shorten keys for a smart graph
// Will usually work up to 2^30 vertices or so.

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"
#include "xxhash64.h"

char const* usage = R"!(Usage:
    hasher vertices vert_out table_out
    hasher edges edge_out

      both read vertices and edges resp. from stdin
)!";

char const* const BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

std::string encodeBase64(char const* in, size_t len) {
  std::string ret;
  ret.reserve((len * 4 / 3) + 2);

  unsigned char charArray3[3];
  unsigned char charArray4[4];
  unsigned char const* bytesToEncode =
      reinterpret_cast<unsigned char const*>(in);

  int i = 0;
  while (len--) {
    charArray3[i++] = *(bytesToEncode++);

    if (i == 3) {
      charArray4[0] = (charArray3[0] & 0xfc) >> 2;
      charArray4[1] =
          ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
      charArray4[2] =
          ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
      charArray4[3] = charArray3[2] & 0x3f;

      for (i = 0; i < 4; i++) {
        ret += BASE64_CHARS[charArray4[i]];
      }

      i = 0;
    }
  }

  if (i != 0) {
    for (int j = i; j < 3; j++) {
      charArray3[j] = '\0';
    }

    charArray4[0] = (charArray3[0] & 0xfc) >> 2;
    charArray4[1] =
        ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
    charArray4[2] =
        ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
    charArray4[3] = charArray3[2] & 0x3f;

    for (int j = 0; (j < i + 1); j++) {
      ret += BASE64_CHARS[charArray4[j]];
    }

#if 0
    // No padding!
    while ((i++ < 3)) {
      ret += '=';
    }
#endif
  }

  return ret;
}

std::string hashfunc(std::string_view key) {
  uint64_t hashval;
  XXHash64 hash(0xdeadbeefdeadbeefull);
  hash.add(&key[0], key.size());
  hashval = htole64(hash.hash());
  return encodeBase64((char*)&hashval, sizeof(uint64_t));
}

std::string hashkey(std::string_view key) {
  std::string newKey;
  auto pos = key.find(':');
  if (pos != std::string::npos) {
    std::string hash = hashfunc(key.substr(pos + 1));
    newKey.reserve(pos + 1 + hash.size());
    newKey.assign(key.substr(0, pos));
    newKey.push_back(':');
    newKey += hash;
  } else {
    newKey = hashfunc(key);
  }
  return newKey;
}

std::string hashedgekey(std::string_view key) {
  std::string newKey;
  auto pos = key.find(':');
  if (pos != std::string::npos) {
    auto pos2 = key.find(':', pos + 1);
    assert(pos2 != std::string::npos);
    std::string hash = hashfunc(key.substr(pos + 1, pos2 - pos - 1));
    newKey.reserve(pos + 1 + hash.size() + 1 + key.size() - pos2 - 1);
    newKey.assign(key.substr(0, pos));
    newKey.push_back(':');
    newKey += hash;
    newKey.push_back(':');
    newKey += key.substr(pos2 + 1);
  } else {
    newKey = hashfunc(key);
  }
  return newKey;
}

int do_vertices(std::string const& outfile, std::string const& tablefile) {
  std::ofstream out(outfile, std::ios::out);
  std::ofstream table(tablefile, std::ios::out);
  std::string line;
  int64_t count = 0;
  while (getline(std::cin, line)) {
    if (!line.empty() && line[0] != '#') {
      VPackBuilder outbuilder;
      try {
        std::shared_ptr<VPackBuilder> builder = VPackParser::fromJson(line);
        outbuilder.clear();
        std::string_view key;
        std::string hash;
        std::string newKey;
#if 0
        std::string_view id;
#endif
        std::size_t pos;
        {
          VPackObjectBuilder ob(&outbuilder);
          for (auto const& p : VPackObjectIterator(builder->slice())) {
            if (p.key.stringView() == "_key") {
              key = p.value.stringView();
              newKey = hashkey(key);
              outbuilder.add(p.key.stringView(), VPackValue(newKey));
            } else if (p.key.stringView() == "_id") {
#if 0
              id = p.value.stringView();
#endif
            } else if (p.key.stringView() == "_rev") {
            } else {
              outbuilder.add(p.key.stringView(), p.value);
            }
          }
#if 0
          if (!id.empty()) {
            std::size_t pos = id.find('/');
            assert(pos != std::string::npos);
            std::string idst;
            idst.reserve(pos + 1 + newKey.size());
            idst.assign(id.substr(0, pos));
            idst.push_back('/');
            idst += newKey;
            outbuilder.add("_id", VPackValue(idst));
          }
#endif
        }
        out << outbuilder.slice().toJson() << "\n";
        table << "{\"_key\":\"" << newKey << "\",\"k\":\"" << key << "\"}\n";
        ++count;
      } catch (std::exception const& exc) {
        std::cerr << "Exception when parsing JSON: " << exc.what() << "\n"
                  << line << "\n";
      }

      if (count % 1000000 == 0) {
        std::cerr << "Have done " << count << " vertices." << std::endl;
      }
    }
  }
  std::cerr << "Have done " << count << " vertices.\nFINISHED." << std::endl;
  return 0;
}

int do_edges(std::string const& outfile) {
  std::ofstream out(outfile, std::ios::out);
  std::string line;
  int64_t count = 0;
  while (getline(std::cin, line)) {
    if (!line.empty() && line[0] != '#') {
      VPackBuilder outbuilder;
      try {
        std::shared_ptr<VPackBuilder> builder = VPackParser::fromJson(line);
        outbuilder.clear();
        std::string newval;
        std::string hash;
        std::size_t pos;
        {
          VPackObjectBuilder ob(&outbuilder);
          for (auto const& p : VPackObjectIterator(builder->slice())) {
            if (p.key.stringView() == "_from" || p.key.stringView() == "_to") {
              auto val = p.value.stringView();
              pos = val.find('/');
              assert(pos != std::string::npos);
              hash = hashkey(val.substr(pos + 1));
              newval.reserve(pos + 1 + hash.size());
              newval = val.substr(0, pos);
              newval.push_back('/');
              newval += hash;
              outbuilder.add(p.key.stringView(), VPackValue(newval));
            } else if (p.key.stringView() == "_key") {
              auto val = p.value.stringView();
              hash = hashedgekey(val);
              outbuilder.add(p.key.stringView(), VPackValue(hash));
            } else if (p.key.stringView() == "_id") {
#if 0
              auto val = p.value.stringView();
              pos = val.find('/');
              assert(pos != std::string::npos);
              hash = hashedgekey(val.substr(pos + 1));
              newval.reserve(pos + 1 + hash.size());
              newval = val.substr(0, pos);
              newval.push_back('/');
              newval += hash;
              outbuilder.add(p.key.stringView(), VPackValue(newval));
#endif
            } else if (p.key.stringView() == "_rev") {
            } else {
              outbuilder.add(p.key.stringView(), p.value);
            }
          }
        }
        out << outbuilder.slice().toJson() << "\n";
        ++count;
      } catch (std::exception const& exc) {
        std::cerr << "Exception when parsing JSON: " << exc.what() << "\n"
                  << line << "\n";
      }

      if (count % 1000000 == 0) {
        std::cerr << "Have done " << count << " edges." << std::endl;
      }
    }
  }
  std::cerr << "Have done " << count << " edges.\nFINISHED." << std::endl;
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cout << usage << std::endl;
    return 0;
  }
  std::string outputfile{argv[2]};
  if (strcmp(argv[1], "vertices") == 0) {
    std::string tablefile;
    if (argc >= 4) {
      tablefile = argv[3];
    } else {
      tablefile = "table.jsonl";
    }
    return do_vertices(outputfile, tablefile);
  } else if (strcmp(argv[1], "edges") == 0) {
    return do_edges(outputfile);
  } else {
    std::cerr << "Unknown mode " << argv[1] << std::endl;
    return 1;
  }
}
