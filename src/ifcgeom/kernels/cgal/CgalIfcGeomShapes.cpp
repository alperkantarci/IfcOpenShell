#include "CgalKernel.h"
#include "CgalConversionResult.h"

bool IfcGeom::CgalKernel::convert(const IfcSchema::IfcManifoldSolidBrep* l, ConversionResults& shape) {
  cgal_shape_t s;
  const SurfaceStyle* collective_style = get_style(l);
  if (convert_shape(l->Outer(),s) ) {
    const SurfaceStyle* indiv_style = get_style(l->Outer());
    
    IfcSchema::IfcClosedShell::list::ptr voids(new IfcSchema::IfcClosedShell::list);
    if (l->is(IfcSchema::Type::IfcFacetedBrepWithVoids)) {
      voids = l->as<IfcSchema::IfcFacetedBrepWithVoids>()->Voids();
    }
#ifdef USE_IFC4
    if (l->is(IfcSchema::Type::IfcAdvancedBrepWithVoids)) {
      voids = l->as<IfcSchema::IfcAdvancedBrepWithVoids>()->Voids();
    }
#endif
    
    for (IfcSchema::IfcClosedShell::list::it it = voids->begin(); it != voids->end(); ++it) {
      //      TopoDS_Shape s2;
      //      /// @todo No extensive shapefixing since shells should be disjoint.
      //      /// @todo Awaiting generalized boolean ops module with appropriate checking
      //      if (convert_shape(l->Outer(), s2)) {
      //        s = BRepAlgoAPI_Cut(s, s2).Shape();
      //      }
    }
    
    shape.push_back(ConversionResult(new CgalShape(s), indiv_style ? indiv_style : collective_style));
    return true;
  }
  return false;
}

bool IfcGeom::CgalKernel::convert(const IfcSchema::IfcMappedItem* l, ConversionResults& shapes) {
  cgal_placement_t gtrsf;
  IfcSchema::IfcCartesianTransformationOperator* transform = l->MappingTarget();
  if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator3DnonUniform) ) {
    Logger::Message(Logger::LOG_ERROR, "Unsupported MappingTarget:", transform->entity);
//    IfcGeom::CgalKernel::convert((IfcSchema::IfcCartesianTransformationOperator3DnonUniform*)transform,gtrsf);
  } else if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator2DnonUniform) ) {
    Logger::Message(Logger::LOG_ERROR, "Unsupported MappingTarget:", transform->entity);
    return false;
  } else if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator3D) ) {
    cgal_placement_t trsf;
    Logger::Message(Logger::LOG_ERROR, "Unsupported MappingTarget:", transform->entity);
//    IfcGeom::CgalKernel::convert((IfcSchema::IfcCartesianTransformationOperator3D*)transform,trsf);
    gtrsf = trsf;
  } else if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator2D) ) {
    cgal_placement_t trsf_2d;
    Logger::Message(Logger::LOG_ERROR, "Unsupported MappingTarget:", transform->entity);
//    IfcGeom::CgalKernel::convert((IfcSchema::IfcCartesianTransformationOperator2D*)transform,trsf_2d);
    gtrsf = (cgal_placement_t) trsf_2d;
  }
  IfcSchema::IfcRepresentationMap* map = l->MappingSource();
  IfcSchema::IfcAxis2Placement* placement = map->MappingOrigin();
  cgal_placement_t trsf;
  if (placement->is(IfcSchema::Type::IfcAxis2Placement3D)) {
    IfcGeom::CgalKernel::convert((IfcSchema::IfcAxis2Placement3D*)placement,trsf);
  } else {
    cgal_placement_t trsf_2d;
    IfcGeom::CgalKernel::convert((IfcSchema::IfcAxis2Placement2D*)placement,trsf_2d);
    trsf = trsf_2d;
  }
  // TODO: Check
  gtrsf = trsf * gtrsf;
  
  const IfcGeom::SurfaceStyle* mapped_item_style = get_style(l);
  
  const size_t previous_size = shapes.size();
  bool b = convert_shapes(map->MappedRepresentation(), shapes);
  
  for (size_t i = previous_size; i < shapes.size(); ++ i ) {
    IfcGeom::CgalPlacement place(gtrsf);
    shapes[i].prepend(&place);
    
    // Apply styles assigned to the mapped item only if on
    // a more granular level no styles have been applied
    if (!shapes[i].hasStyle()) {
      shapes[i].setStyle(mapped_item_style);
    }
  }
  
  return b;
}

bool IfcGeom::CgalKernel::convert(const IfcSchema::IfcExtrudedAreaSolid *l, cgal_shape_t &shape) {
  const double height = l->Depth() * getValue(GV_LENGTH_UNIT);
  if (height < getValue(GV_PRECISION)) {
    Logger::Message(Logger::LOG_ERROR, "Non-positive extrusion height encountered for:", l->entity);
    return false;
  }
  
  cgal_face_t face;
  if ( !convert_face(l->SweptArea(),face) ) return false;
  
  cgal_placement_t trsf;
  bool has_position = true;
#ifdef USE_IFC4
  has_position = l->hasPosition();
#endif
  if (has_position) {
    IfcGeom::CgalKernel::convert(l->Position(), trsf);
  }
  
  cgal_direction_t dir;
  convert(l->ExtrudedDirection(),dir);
  //  std::cout << "Direction: " << dir << std::endl;
  
  std::list<cgal_face_t> face_list;
  face_list.push_back(face);
  
  for (std::vector<Kernel::Point_3>::const_iterator current_vertex = face.outer.begin();
       current_vertex != face.outer.end();
       ++current_vertex) {
    std::vector<Kernel::Point_3>::const_iterator next_vertex = current_vertex;
    ++next_vertex;
    if (next_vertex == face.outer.end()) {
      next_vertex = face.outer.begin();
    } cgal_face_t side_face;
    side_face.outer.push_back(*next_vertex);
    side_face.outer.push_back(*current_vertex);
    side_face.outer.push_back(*current_vertex+height*dir);
    side_face.outer.push_back(*next_vertex+height*dir);
    face_list.push_back(side_face);
  }
  
  cgal_face_t top_face;
  for (std::vector<Kernel::Point_3>::const_reverse_iterator vertex = face.outer.rbegin();
       vertex != face.outer.rend();
       ++vertex) {
    top_face.outer.push_back(*vertex+height*dir);
  } face_list.push_back(top_face);
  
  // Naive creation
  cgal_shape_t polyhedron = CGAL::Polyhedron_3<Kernel>();
  PolyhedronBuilder builder(&face_list);
  polyhedron.delegate(builder);
  
  // Stitch edges
  //  std::cout << "Before: " << polyhedron.size_of_vertices() << " vertices and " << polyhedron.size_of_facets() << " facets" << std::endl;
  CGAL::Polygon_mesh_processing::stitch_borders(polyhedron);
  if (!CGAL::Polygon_mesh_processing::is_outward_oriented(polyhedron)) {
    CGAL::Polygon_mesh_processing::reverse_face_orientations(polyhedron);
  }
  //  std::cout << "After: " << polyhedron.size_of_vertices() << " vertices and " << polyhedron.size_of_facets() << " facets" << std::endl;
  
  shape = polyhedron;
  return true;
}

bool IfcGeom::CgalKernel::convert(const IfcSchema::IfcConnectedFaceSet* l, cgal_shape_t& shape) {
  IfcSchema::IfcFace::list::ptr faces = l->CfsFaces();
  
  std::list<cgal_face_t> face_list;
  for (IfcSchema::IfcFace::list::it it = faces->begin(); it != faces->end(); ++it) {
    bool success = false;
    cgal_face_t face;
    
    try {
      success = convert_face(*it, face);
    } catch (...) {}
    
    if (!success) {
      Logger::Message(Logger::LOG_WARNING, "Failed to convert face:", (*it)->entity);
      continue;
    }
    
    //    std::cout << "Face in ConnectedFaceSet: " << std::endl;
    //    for (auto &point: face.outer) {
    //      std::cout << "\tPoint(" << point << ")" << std::endl;
    //    }
    
    face_list.push_back(face);
  }
  
  // Naive creation
  cgal_shape_t polyhedron = CGAL::Polyhedron_3<Kernel>();
  PolyhedronBuilder builder(&face_list);
  polyhedron.delegate(builder);
  
  // Stitch edges
  //  std::cout << "Before: " << polyhedron.size_of_vertices() << " vertices and " << polyhedron.size_of_facets() << " facets" << std::endl;
  CGAL::Polygon_mesh_processing::stitch_borders(polyhedron);
  if (!CGAL::Polygon_mesh_processing::is_outward_oriented(polyhedron)) {
    CGAL::Polygon_mesh_processing::reverse_face_orientations(polyhedron);
  }
  //  std::cout << "After: " << polyhedron.size_of_vertices() << " vertices and " << polyhedron.size_of_facets() << " facets" << std::endl;
  
  shape = polyhedron;
  return true;
}

bool IfcGeom::CgalKernel::convert(const IfcSchema::IfcCsgSolid* l, cgal_shape_t& shape) {
  return convert_shape(l->TreeRootExpression(), shape);
}

bool IfcGeom::CgalKernel::convert(const IfcSchema::IfcBlock* l, cgal_shape_t& shape) {
  const double dx = l->XLength() * getValue(GV_LENGTH_UNIT);
  const double dy = l->YLength() * getValue(GV_LENGTH_UNIT);
  const double dz = l->ZLength() * getValue(GV_LENGTH_UNIT);
  
  std::list<cgal_face_t> face_list;
  
  // x = 0
  face_list.push_back(cgal_face_t());
  face_list.back().outer.push_back(Kernel::Point_3(0, 0, 0));
  face_list.back().outer.push_back(Kernel::Point_3(0, dy, 0));
  face_list.back().outer.push_back(Kernel::Point_3(0, dy, dz));
  face_list.back().outer.push_back(Kernel::Point_3(0, 0, dz));
  
  // x = dx
  face_list.push_back(cgal_face_t());
  face_list.back().outer.push_back(Kernel::Point_3(dx, 0, 0));
  face_list.back().outer.push_back(Kernel::Point_3(dx, 0, dz));
  face_list.back().outer.push_back(Kernel::Point_3(dx, dy, dz));
  face_list.back().outer.push_back(Kernel::Point_3(dx, dy, 0));
  
  // y = 0
  face_list.push_back(cgal_face_t());
  face_list.back().outer.push_back(Kernel::Point_3(0, 0, 0));
  face_list.back().outer.push_back(Kernel::Point_3(0, 0, dz));
  face_list.back().outer.push_back(Kernel::Point_3(dx, 0, dz));
  face_list.back().outer.push_back(Kernel::Point_3(dx, 0, 0));
  
  // y = dy
  face_list.push_back(cgal_face_t());
  face_list.back().outer.push_back(Kernel::Point_3(0, dy, 0));
  face_list.back().outer.push_back(Kernel::Point_3(dx, dy, 0));
  face_list.back().outer.push_back(Kernel::Point_3(dx, dy, dz));
  face_list.back().outer.push_back(Kernel::Point_3(0, dy, dz));
  
  // z = 0
  face_list.push_back(cgal_face_t());
  face_list.back().outer.push_back(Kernel::Point_3(0, 0, 0));
  face_list.back().outer.push_back(Kernel::Point_3(dx, 0, 0));
  face_list.back().outer.push_back(Kernel::Point_3(dx, dy, 0));
  face_list.back().outer.push_back(Kernel::Point_3(0, dy, 0));
  
  // z = dz
  face_list.push_back(cgal_face_t());
  face_list.back().outer.push_back(Kernel::Point_3(0, 0, dz));
  face_list.back().outer.push_back(Kernel::Point_3(0, dy, dz));
  face_list.back().outer.push_back(Kernel::Point_3(dx, dy, dz));
  face_list.back().outer.push_back(Kernel::Point_3(dx, 0, dz));
  
  // Naive creation
  cgal_shape_t polyhedron = CGAL::Polyhedron_3<Kernel>();
  PolyhedronBuilder builder(&face_list);
  polyhedron.delegate(builder);
  
  // Stitch edges
  //  std::cout << "Before: " << polyhedron.size_of_vertices() << " vertices and " << polyhedron.size_of_facets() << " facets" << std::endl;
  CGAL::Polygon_mesh_processing::stitch_borders(polyhedron);
  if (!CGAL::Polygon_mesh_processing::is_outward_oriented(polyhedron)) {
    CGAL::Polygon_mesh_processing::reverse_face_orientations(polyhedron);
  }
  //  std::cout << "After: " << polyhedron.size_of_vertices() << " vertices and " << polyhedron.size_of_facets() << " facets" << std::endl;
  
  cgal_placement_t trsf;
  IfcGeom::CgalKernel::convert(l->Position(),trsf);

  // IfcCsgPrimitive3D.Position has unit scale factor
  for (auto &vertex: vertices(polyhedron)) {
    vertex->point() = vertex->point().transform(trsf);
  }

  shape = polyhedron;
  return true;
}
