# IfcOpenShell - IFC toolkit and geometry engine
# Copyright (C) 2021 Dion Moult <dion@thinkmoult.com>
#
# This file is part of IfcOpenShell.
#
# IfcOpenShell is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# IfcOpenShell is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with IfcOpenShell.  If not, see <http://www.gnu.org/licenses/>.

import pytest
import test.bootstrap
import ifcopenshell.api
import ifcopenshell.util.element


class TestUnassignObject(test.bootstrap.IFC4):
    def test_unassigning_an_object(self):
        element = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        subelement1 = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        subelement2 = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        ifcopenshell.api.run(
            "nest.assign_object", self.file, related_objects=[subelement1, subelement2], relating_object=element
        )
        ifcopenshell.api.run("nest.unassign_object", self.file, related_objects=[subelement1, subelement2])
        assert ifcopenshell.util.element.get_nest(subelement1) is None
        assert ifcopenshell.util.element.get_nest(subelement2) is None

    def test_the_rel_is_kept_if_there_are_more_nested_elements(self):
        element = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        subelement2 = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        subelement1 = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        ifcopenshell.api.run("nest.assign_object", self.file, related_objects=[subelement1], relating_object=element)
        ifcopenshell.api.run("nest.assign_object", self.file, related_objects=[subelement2], relating_object=element)
        ifcopenshell.api.run("nest.unassign_object", self.file, related_objects=[subelement1])
        assert len(self.file.by_type("IfcRelNests")) == 1

    def test_the_rel_is_purged_if_there_are_no_more_nested_elements(self):
        element = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        subelement = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcTask")
        ifcopenshell.api.run("nest.assign_object", self.file, related_objects=[subelement], relating_object=element)
        ifcopenshell.api.run("nest.unassign_object", self.file, related_objects=[subelement])
        assert len(self.file.by_type("IfcRelNests")) == 0
