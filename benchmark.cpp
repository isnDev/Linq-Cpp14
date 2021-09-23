#include <cstdlib>
#include <iostream>
#include <ctime>

#include "linq/linq.h"
#include "assert.h"

struct heavy {
    int map[1024];

    heavy() {
        for (auto& x : map)
            x = (int)(std::rand() % 1000);
    }
};

struct light {
    int map[1];

    light() {
        for (auto& x : map)
            x = (int)(std::rand() % 1000);
    }
};

struct user
{
    int groupId;
    int created;

    user() {
        groupId = (int)(std::rand() % 10);
        created = (int)(std::rand() % 2);
    }
};

template <typename T>
void bench()
{
    std::vector<T> cont(200000);
    auto const& data = cont;

    // benchmark tests

    std::cout << "Select.Sum" << std::endl;
    auto x1 = test("->IEnumerable", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept { return val.map[0]; })
            .Sum();
        });
    auto x2 = test("->Legacy", [&]() {
        int result = 0;
        for (const auto& it : data)
            result += it.map[0];
        return result;
        });

    auto x111 = test("->IEnumerable", [&]() {
        return linq::make_enumerable(data)
            .Take(1000)
            .Select([](const auto& val) noexcept -> const auto& { return val.map[0]; })
            .Sum();
        });
    auto x222 = test("->Legacy", [&]() {
        int result = 0;
        for (int i = 0; i < 1000; ++i)
            result += data[i].map[0];
        return result;
        });

    assertEquals(x1, x2);
    assertEquals(x111, x222);

    std::cout << "Select.Where.Sum" << std::endl;
    auto x3 = test("->IEnumerable", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto& { return val.map[0];; })
            .Where([](const auto& val) noexcept { return val > 5; })
            .Sum();
        });
    auto x100 = test("->NotIEnumerable", [&]() {
        return linq::from(data)
            .select([](const auto& val) noexcept -> const auto& { return val.map[0];; })
            .where([](const auto& val) noexcept { return val > 5; })
            .sum();
        });
    auto x4 = test("->Legacy", [&]() {
        int result = 0;
        for (const auto& it : data)
        {
            const auto val = it.map[0];
            if (val > 5)
                result += val;
        }
        return result;
        });
    assertEquals(x3, x4);

    std::cout << "Select.Skip.Take.Take.Skip.Sum" << std::endl;
    auto x5 = test("->IEnumerable", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto& { return val.map[0]; })
            .Skip(20000)
            .Take(500000)
            .Take(190000)
            .Skip(20000)
            .Sum();
        });
    auto x6 = test("->Legacy", [&]() {
        int result = 0;
        auto it = data.cbegin();
        for (int i = 0; i < 40000; ++i, ++it);
        for (int i = 0; i < 160000 && it != data.cend(); ++i, ++it)
        {
            const auto val = it->map[0];
            result += val;
        }
        return result;
        });
    auto x7 = test("->IEnumerable (Optimal Pattern)", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto& { return val.map[0]; })
            .Skip(40000)
            .Take(160000)
            .Sum();
        });
    assertEquals(x5, x6);
    assertEquals(x5, x7);

    std::cout << "Select.Skip.Take.Take.Skip.Where.Sum" << std::endl;
    auto x8 = test("->IEnumerable", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto  { return val.map[0]; })
            .Skip(20000)
            .Take(500000)
            .Take(190000)
            .Skip(20000)
            .Where([](const auto& val) noexcept { return val > 5; })
            .Sum();
        });
    auto x9 = test("->Legacy", [&]() {
        int result = 0;
        auto it = data.cbegin();
        for (int i = 0; i < 40000; ++i, ++it);
        auto j = 0;
        for (int i = 0; i < 160000; ++i, ++it)
        {
            const auto val = it->map[0];
            if (val > 5) {
                result += val;
                j++;
            }
        }
        return result;
        });
    auto x10 = test("->IEnumerable (Optimal Pattern)", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto  { return val.map[0]; })
            .Skip(40000)
            .Take(160000)
            .Where([](const auto& val) noexcept { return val > 5; })
            .Sum();
        });
    assertEquals(x8, x9);
    assertEquals(x8, x10);

    std::cout << "Select.Where.Skip.Take.Take.Skip.Sum" << std::endl;
    auto x11 = test("->IEnumerable", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto  { return val.map[0]; })
            .Where([](const auto& val) noexcept { return val > 5; })
            .Skip(20000)
            .Take(500000)
            .Take(190000)
            .Skip(20000)
            .Sum();
        });
    auto x12 = test("->Legacy", [&]() {
        int result = 0;
        const int limit = 190000;
        const int skip = 40000;
        int i = 0;
        int took = 0;
        for (auto it = data.cbegin(); took < limit && it != data.cend(); ++it)
        {
            const auto val = it->map[0];
            if (val > 5)
            {
                if (i >= skip)
                {
                    ++took;
                    result += val;
                }
                ++i;
            }
        }
        return result;
        });
    auto x13 = test("->IEnumerable (Optimal Pattern)", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto  { return val.map[0]; })
            .Where([](const auto& val) noexcept { return val > 5; })
            .Skip(40000)
            .Take(160000)
            .Sum();
        });
    assertEquals(x11, x12);
    assertEquals(x11, x13);
    std::cout << std::endl;

    auto x16 = test("StressTest->IEnumerable (GroupBy)", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto  { return val.map[0]; })
            .Where([](const auto& val) noexcept { return val > 5; })
            .Skip(40000)
            .Take(160000)
            .GroupBy([](const auto& key) { return key; }, [](const auto& key) { return key; })
            .Select([](auto& pair) {
            return linq::make_enumerable(pair.second[pair.first])
                .Sum(); })
            .Sum();
        });

    auto x17 = test("StressTest->IEnumerable (GroupBy.ThenBy)", [&]() {
        return linq::make_enumerable(data)
            .Select([](const auto& val) noexcept -> const auto { return val.map[0]; })
            .Where([](const auto& val) noexcept { return val > 5; })
            .Skip(40000)
            .Take(160000)
            .GroupBy([](const auto& key) -> const auto { return key; })
            //.ThenBy([](const auto &key) -> const auto { return key; })
            .Select([](const auto& pair) { return linq::make_enumerable(pair.second).Sum(); })
            .Sum();
        });

    assertEquals(x13, x16);
    assertEquals(x13, x17);

}

#include <unordered_map>

template <>
void bench<user>()
{
    std::vector<user> data(200000);

    auto x0 = test("->IEnumerable (GroupBy)", [&]() {
        return linq::make_enumerable(data)
            .Where([](const auto& val) noexcept { return val.groupId > 5; })
            .GroupBy(
                [](const auto& key) noexcept { return key.groupId; },
                [](const auto& key) { return key.created; }).First().first;
        });

    auto x1 = test("->Legacy (GroupBy)", [&]() {
        std::unordered_map<int, std::unordered_map<int, std::vector<user>>> group;
        for (const auto& it : data)
            if (it.groupId > 5)
                group[it.groupId][it.created].push_back(it);
        return group.begin()->first;
        });

    assertEquals(x0, x1);

    auto x2 = test("->IEnumerable (OrderBy)", [&]() {
        return linq::make_enumerable(data)
            .Where([](const auto& val) noexcept { return val.groupId > 6; })
            .OrderBy(linq::asc([](const auto& key) noexcept { return key.groupId; })).First();
        });

    auto x3 = test("->IEnumerable (OrderBy)", [&]() {
        return linq::make_enumerable(data)
            .Where([](const auto& val) noexcept { return val.groupId > 5; })
            .Where([](const auto& val) noexcept { return val.groupId > 6; })
            .OrderBy(
                linq::asc([](const auto& key) { return key.groupId; }),
                linq::desc([](const auto& key) { return key.created; }))
            .Where([](const auto& val) noexcept { return val.groupId > 7; })
            .SelectMany([](const auto& val) noexcept { return val.groupId; },
                [](const auto& val) noexcept { return val.created; })
            .Select([](const auto& val) noexcept { return std::get<0>(val); })
            .Where([](const auto& val) noexcept { return val > 7; })
            .All()
            .Sum();
        });

    //auto x3 = test("->IEnumerable (OrderBy)", [&]() {
    //    return linq::make_enumerable(data)
    //        .Where([](const auto &val) noexcept { return val.groupId > 5; })
    //        .OrderBy(linq::make_asc([](const auto &key) noexcept { return key.groupId; }));
    //});

    std::cout << std::endl;
}

int main(int, char* [])
{
    std::srand(time(0));
    std::cout << "# Light objects" << std::endl;
    bench<light>();

    std::cout << "# Heavy objects" << std::endl;
    bench<heavy>();

    std::cout << "# User objects" << std::endl;
    bench<user>();

    return EXIT_SUCCESS;
}
