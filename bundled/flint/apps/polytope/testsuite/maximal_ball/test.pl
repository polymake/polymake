compare_data("1", maximal_ball(load("1.poly")));
compare_data("2", maximal_ball(load("2.poly")));
compare_data("3", maximal_ball(load("3.poly")));
compare_data("square", maximal_ball(cube(2,1,0)));
compare_data("dilated_triangle", maximal_ball(simplex(2,4/5)));
compare_data("dilated_4cube", maximal_ball(cube(4,7,2)));
