#include <iostream>
#include <vector>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Circle_2.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel K;
typedef K::Point_2 Point_2;
typedef CGAL::Polygon_2<K> Polygon_2;
typedef CGAL::Circle_2<K> Circle_2;

int main() {
    // Definir un polígono (por ejemplo, un triángulo)
    std::vector<Point_2> polygon_points;
    polygon_points.push_back(Point_2(1, 1));
    polygon_points.push_back(Point_2(3, 1));
    polygon_points.push_back(Point_2(2, 3));
    Polygon_2 polygon(polygon_points.begin(), polygon_points.end());

    // Definir un círculo
    Circle_2 circle(Point_2(2, 2), 1); // Centro (2, 2), radio 1

    // Calcular la intersección entre el polígono y el círculo
    std::list<Polygon_2> intersection_polys;
    CGAL::intersection(polygon, circle, std::back_inserter(intersection_polys));

    //// Calcular el área total de la intersección
    //double total_area = 0;
    //for (const auto& poly : intersection_polys) {
        //total_area += poly.area();
    //}

    //std::cout << "Área de intersección entre el polígono y el círculo: " << total_area << std::endl;

    return 0;
}

