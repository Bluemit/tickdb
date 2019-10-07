#include "timeline.h"

#include "util.h"

namespace TickDB {


Timeline::Timeline() {}

Timeline::~Timeline() {}

void Timeline::insert(uint64_t ts, const Slice& row) {
    auto it = _ts_data_map.find(ts);
    _ts_data_map[ts] = row; 
    if (it != _ts_data_map.end()) {
        return;
    }

    Util::sort_insert(_ts_index, ts);
}

const Slice* Timeline::get(uint64_t ts) {
    auto it = _ts_data_map.find(ts);
    return it == _ts_data_map.end() ? NULL : &(it->second);
}

const Slice* Timeline::at(int index) {
    unsigned int len = _ts_index.size();
    if (index < 0) {
        index = len + index;
        if (index < 0) {
            //index = 0;
            return NULL;
        }
    } else if (index >= len) {
        //index = len - 1;
        return NULL;
    }
    return &(_ts_data_map[_ts_index[index]]);
}

int Timeline::get_index_by_time(uint64_t ts, QueryOP op) {
    int pos = -1;
    switch (op) {
        case QueryOP::EQ:
            pos = Util::binary_search(_ts_index, ts, Util::BinarySearchEqualFlag);
            break;
        case QueryOP::GT:
            pos = Util::binary_search(_ts_index, ts, Util::BinarySearchGTFlag);
            break;
        case QueryOP::GE:
            pos = Util::binary_search(_ts_index, ts, Util::BinarySearchGEFlag);
            break;
        case QueryOP::LT:
            pos = Util::binary_search(_ts_index, ts, Util::BinarySearchLTFlag);
            break;
        case QueryOP::LE:
            pos = Util::binary_search(_ts_index, ts, Util::BinarySearchLEFlag);
            break;
        default:
            pos = -1;
    }
    if (pos >= _ts_index.size()) {
        pos = -1;
    }
    return pos;
}

bool Timeline::shift_left(uint64_t ts, int n, std::vector<const Slice*>& rows) {
    if (n <= 0) {
        return true;
    }
    int pos = get_index_by_time(ts, QueryOP::LT);
    if (pos == -1) {
        return false;
    }
    int index_start = pos - n + 1;
    if (index_start < 0) {
        index_start = 0;
    }
    return index_range(index_start, pos, rows);
}

bool Timeline::shift_right(uint64_t ts, int n, std::vector<const Slice*>& rows) {
    if (n <= 0) {
        return true;
    }
    int pos = get_index_by_time(ts, QueryOP::GT);
    if (pos == -1) {
        return false;
    }
    int index_end = pos + n - 1;
    if (index_end >= _ts_index.size()) {
        index_end = _ts_index.size() - 1;
    }
    return index_range(pos, index_end, rows);
}

const Slice* Timeline::gt(uint64_t ts) {
    int pos = Util::binary_search(_ts_index, ts, Util::BinarySearchGTFlag);
    if (pos >= _ts_index.size()) {
        return NULL;
    }
    return at(pos); 
}

const Slice* Timeline::ge(uint64_t ts) {
    int pos = Util::binary_search(_ts_index, ts, Util::BinarySearchGEFlag);
    if (pos >= _ts_index.size()) {
        return NULL;
    }
    return at(pos); 
}

const Slice* Timeline::lt(uint64_t ts) {
    int pos = Util::binary_search(_ts_index, ts, Util::BinarySearchLTFlag);
    if (pos < 0) {
        return NULL;
    }
    return at(pos); 
}

const Slice* Timeline::le(uint64_t ts) {
    int pos = Util::binary_search(_ts_index, ts, Util::BinarySearchLEFlag);
    if (pos < 0) {
        return NULL;
    }
    return at(pos); 
}

std::vector<const Slice*> Timeline::del(uint64_t start, uint64_t end) {
    std::vector<const Slice*> ret;
    if (start > end) {
        return std::move(ret);
    }
    int pos_start = Util::binary_search(_ts_index, start, Util::BinarySearchGEFlag);
    if (pos_start >= _ts_index.size()) {
        return std::move(ret);
    }
    int pos_end = Util::binary_search(_ts_index, end, Util::BinarySearchLEFlag, pos_start);
    if (pos_end < 0) {
        return std::move(ret);
    }

    //std::cout << "==== pos_start:" << pos_start << " pos_end:" << pos_end << std::endl;

    for (int i = pos_start; i <= pos_end; ++i) {
        ret.push_back(&(_ts_data_map[_ts_index[i]]));
        _ts_data_map.erase(_ts_index[i]);
    }
    _ts_index.erase(_ts_index.begin() + pos_start, _ts_index.begin() + pos_end + 1);

    return std::move(ret);
}

void Timeline::del(uint64_t ts) {
    int pos = Util::binary_search(_ts_index, ts, Util::BinarySearchEqualFlag);
    if (pos != -1) {
        uint64_t ts = _ts_index[pos];
        _ts_index.erase(_ts_index.begin() + pos);
        _ts_data_map.erase(ts);

    }
}

bool Timeline::index_range(int start, int end, std::vector<const Slice*>& rows) {
    if (start < 0) {
        start += _ts_index.size();
    }
    if (end < 0) {
        end += _ts_index.size();
    }
    if (start > end || start < 0 || end >= _ts_index.size()) {
        return false;
    }
    for (int i = start; i <= end; ++i) {
        rows.push_back(&(_ts_data_map[_ts_index[i]]));
    }

    return true;
}

bool Timeline::time_range(uint64_t start, uint64_t end, std::vector<const Slice*>& rows) {
    if (start > end) {
        return false;
    }
    int pos_start = Util::binary_search(_ts_index, start, Util::BinarySearchGEFlag);
    if (pos_start >= _ts_index.size()) {
        return false;
    }
    int pos_end = Util::binary_search(_ts_index, end, Util::BinarySearchLEFlag, pos_start);
    if (pos_end < 0) {
        return false;
    }

    for (int i = pos_start; i <= pos_end; ++i) {
        rows.push_back(&(_ts_data_map[_ts_index[i]]));
    }

    return true;
}

} // namespace TickDB
