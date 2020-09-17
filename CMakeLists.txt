cmake_minimum_required(VERSION 3.13)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_HEADERS
        test_runner.h
        profile.h
        #        condition_parser.h
        #        database.h
        #        date.h
        #        node.h
        #        Red_1_week/airline_ticket.h
        #        token.h
        #        Red_3_week/simple_vector.h
        #        stack_vector.h
        #        http_request.h
        #        stats.h
        #        simple_vector_2.h
        #        Red_course_project/iterator_range.h
        #        Red_course_project/parse.h
        #        Red_course_project/search_server.h
        #        ini.h
        #        xml.h
        #        json.h
        #        geo2d.h
        #        game_object.h
        #        Brown_3_week/Common.h
        #        Textures.h
        #        Common_textures.h
        #        Common.h
        #        Brown_4_week/old_booking_providers.h
        #        Brown_4_week/old_trip_manager.h
        #        Brown_4_week/new_booking_providers.h
        #        Brown_4_week/new_trip_manager.h
        #        Brown_4_week/booking.h
        #        json.h
        #        Brown/Brown_course_project/graph.h
        #        Brown/Brown_course_project/router.h
        #        Brown/Brown_course_project/my_json.h
        )
set(PROJECT_SOURCES
        #        condition_parser.cpp
        #        database.cpp
        #        date.cpp
        #        node.cpp
        #        token.cpp
        #        Red_1_week/table.cpp
        #        Red_1_week/sort_by.cpp
        #        Red_1_week/uniq_id.cpp
        #        Red_1_week/update_field.cpp
        #        Red_1_week/deque.cpp
        #        Red_1_week/paginator.cpp
        #        Red_2_week/e-book-fast.cpp
        #        Red_2_week/booking.cpp
        #        Red_3_week/object_pool.cpp
        #        Red_3_week/swap_sort_copy.cpp
        #        Red_3_week/simple_vector.cpp
        #        Red_3_week/linked_list.cpp
        #        athlets.cpp
        #        stack_vector.cpp
        #        translator.cpp
        #        airport_counter.cpp
        #        text_editor.cpp
        #        stats.cpp
        #        josephus_permutation.cpp
        #        group_heavy_string.cpp
        #        split_into_sentences.cpp
        #        simple_vector_2.cpp
        #        merge_sort_move.cpp
        #        Red/Red_5_week/priority_collection.cpp
        #        matrix_sum.cpp
        #        explore_key_words.cpp
        #        synchronized.cpp
        #        Red_5_week/concurrent_map.cpp
        #        Red_course_project/main.cpp
        #        Red_course_project/parse.cpp
        #        Red_course_project/search_server.cpp
        #        hash_set.cpp
        #        set_iterator_next.cpp
        #        hash_point.cpp
        #        hash_person.cpp
        #        Brown_1_week/secondary_index.cpp
        #        spendings_xml.cpp
        #        spendings_json.cpp
        #        ini.cpp
        #        test_ini.cpp
        #        xml.cpp
        #        json.cpp
        #        refactoring.cpp
        #        geo2d.cpp
        #        collide.cpp
        #        comment_server.cpp
        #        print_stats_2.cpp
        #        lazy_value.cpp
        #        concurrent_map_2.cpp
        #        Brown_3_week/polynomial.cpp
        #        Brown_3_week/main_tree.cpp
        #        Brown_3_week/pipeline.cpp
        #        Textures.cpp
        #        main_textures.cpp
        #        Solution_textures.cpp
        #        main.cpp
        #        Solution.cpp
                Brown/Brown_4_week/unique_ptr.cpp
        #        Brown_4_week/test.cpp
        #        print_stats_function_with_tests.cpp
        #        expresses.cpp
        #        team_tasks.cpp
        #        domains.cpp
        #        test_domains.cpp
        #        Brown_5_week/budget.cpp
        #        Brown_5_week/budget_good_base.cpp
        #        Brown_5_week/budget_bad_base.cpp
        #        Brown_5_week/budget_bad_base_old.cpp
        #        Brown_5_week/budget_good_base_old.cpp
        #        json.cpp
        #        Brown/Brown_course_project/my_json.cpp
        #        Brown/Brown_course_project/RouteManager_2.cpp
        #        Brown/Brown_5_week/budget.cpp
        )

add_executable(untitled ${PROJECT_SOURCES} ${PROJECT_HEADERS})