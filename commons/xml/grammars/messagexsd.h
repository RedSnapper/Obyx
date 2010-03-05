std::string Parser::messagexsd=""
"<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:m=\"http://www.obyx.org/message\""
"    targetNamespace=\"http://www.obyx.org/message\" elementFormDefault=\"qualified\""
"    attributeFormDefault=\"unqualified\" version=\"v1.10.01.11\">"
"	<xs:annotation>"
"		<xs:documentation>"
"			message.xsd is authored and maintained by Ben Griffin of Red Snapper Ltd."
"			message.xsd is a part of Obyx - see http://www.obyx.org ."
"			Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd."
"			This file is Copyright (C) 2006-2009 Red Snapper Ltd. http://www.redsnapper.net"
"			The governing usage license can be found at http://www.gnu.org/licenses/gpl-3.0.txt"
"			"
"			This program is free software; you can redistribute it and/or modify it"
"			under the terms of the GNU General Public License as published by the Free"
"			Software Foundation; either version 3 of the License, or (at your option)"
"			any later version."
"			"
"			This program is distributed in the hope that it will be useful, but WITHOUT"
"			ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or"
"			FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for"
"			more details."
"			"
"			You should have received a copy of the GNU General Public License along"
"			with this program; if not, write to the Free Software Foundation, Inc.,"
"			59 Temple Place, Suite 330, Boston, MA  02111-1307 USA"
"		</xs:documentation>"
"	</xs:annotation>"
"	<xs:element name=\"message\">"
"        <xs:complexType mixed=\"false\">"
"            <xs:sequence>"
"            	<xs:element name=\"header\" minOccurs=\"0\" maxOccurs=\"unbounded\">"
"            		<xs:complexType mixed=\"false\">"
"            			<xs:sequence minOccurs=\"0\">"
"            				<xs:element name=\"address\" minOccurs=\"0\" maxOccurs=\"unbounded\" type=\"m:address_type\" />"
"            				<xs:element name=\"comment\" minOccurs=\"0\" maxOccurs=\"unbounded\" type=\"m:comment_type\" />"
"            				<xs:element name=\"subhead\"  minOccurs=\"0\" maxOccurs=\"unbounded\">"
"            					<xs:complexType mixed=\"false\">"
"            						<xs:sequence minOccurs=\"0\">"
"            							<xs:element name=\"address\" minOccurs=\"0\" maxOccurs=\"unbounded\" type=\"m:address_type\" />"
"            							<xs:element name=\"comment\" minOccurs=\"0\" maxOccurs=\"unbounded\" type=\"m:comment_type\" />"
"            						</xs:sequence>"
"            						<xs:attribute name=\"name\" use=\"optional\" type=\"xs:string\"/>"
"            						<xs:attribute name=\"value\" use=\"optional\" type=\"xs:string\"/>"
"            						<xs:attributeGroup ref=\"m:modifiers\"/>"
"            					</xs:complexType>"
"            				</xs:element>"
"            			</xs:sequence>"
"            			<xs:attribute name=\"name\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attribute name=\"value\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attribute name=\"cookie\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attributeGroup ref=\"m:modifiers\"/>"
"            		</xs:complexType>"
"            	</xs:element>"
"            	<xs:element name=\"body\" minOccurs=\"0\" maxOccurs=\"1\">"
"            		<xs:complexType mixed=\"true\">"
"            			<xs:choice minOccurs=\"0\">"
"            				<xs:element ref=\"m:message\" minOccurs=\"0\" maxOccurs=\"unbounded\"/>"
"            			</xs:choice>"
"            			<xs:attribute name=\"mechanism\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attribute name=\"type\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attribute name=\"subtype\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attribute name=\"format\" use=\"optional\" type=\"xs:string\"/>"
"            			<xs:attribute name=\"charset\" use=\"optional\" type=\"xs:string\"/>"
"						<xs:attribute name=\"urlencoded\" use=\"optional\" type=\"xs:boolean\"/>"
"            		</xs:complexType>"
"            	</xs:element>"
"            </xs:sequence>"
"            <xs:attribute name=\"id\" use=\"optional\" type=\"xs:ID\"/>"
"        </xs:complexType>"
"    </xs:element>"
"	<xs:complexType mixed=\"false\" name=\"address_type\" >"
"		<xs:attribute name=\"note\" use=\"optional\" type=\"xs:string\"/>"
"		<xs:attribute name=\"value\" use=\"required\" type=\"xs:string\"/>"
"		<xs:attributeGroup ref=\"m:modifiers\"/>"
"	</xs:complexType>"
"	<xs:complexType name=\"comment_type\" mixed=\"false\">"
"		<xs:attribute name=\"value\" use=\"required\" type=\"xs:string\"/>"
"		<xs:attributeGroup ref=\"m:modifiers\"/>"
"	</xs:complexType>"
"	<xs:attributeGroup name=\"modifiers\">"
"		<xs:attribute name=\"urlencoded\" use=\"optional\" type=\"xs:boolean\"/>"
"		<xs:attribute name=\"angled\" use=\"optional\" type=\"xs:boolean\"/>"
"	</xs:attributeGroup>	"
"</xs:schema>"
"";