# IfcOpenShell - IFC toolkit and geometry engine
# Copyright (C) 2022 Dion Moult <dion@thinkmoult.com>
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

import test.bootstrap
import ifcopenshell.api


class TestAssignGroup(test.bootstrap.IFC4):
    def test_assign_group_for_multiple_elements(self):
        element = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcPump")
        element2 = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcPump")
        group = ifcopenshell.api.run("group.add_group", self.file)

        # assign group for multiple elements
        ifcopenshell.api.run("group.assign_group", self.file, products=[element, element2], group=group)
        assert len(rels := self.file.by_type("IfcRelAssignsToGroup")) == 1
        rel = rels[0]
        assert rel.RelatingGroup == group
        assert rel.RelatedObjects == (element, element2)

    def test_reuse_existing_relationship(self):
        self.test_assign_group_for_multiple_elements()
        rel = self.file.by_type("IfcRelAssignsToGroup")[0]
        element = ifcopenshell.api.run("root.create_entity", self.file, ifc_class="IfcPump")
        group = self.file.by_type("IfcGroup")[0]
        ifcopenshell.api.run("group.assign_group", self.file, products=[element], group=group)

        assert len(self.file.by_type("IfcRelAssignsToGroup")) == 1
        assert rel.RelatingGroup == group
        assert set(rel.RelatedObjects) == set(self.file.by_type("IfcPump"))
