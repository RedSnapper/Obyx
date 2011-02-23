std::string Parser::obyxxsd=""
"<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:o=\"http://www.obyx.org\""
"	targetNamespace=\"http://www.obyx.org\" elementFormDefault=\"qualified\""
"	attributeFormDefault=\"unqualified\" version=\"v1.110223\" >"
"	<xs:annotation><xs:documentation>"
"		obyx.xsd is authored and maintained by Ben Griffin of Red Snapper Ltd."
"		obyx.xsd is a part of Obyx - see http://www.obyx.org ."
"		Obyx is protected as a trade mark (2483369) in the name of Red Snapper Ltd."
"		This file is Copyright (C) 2006-2010 Red Snapper Ltd. http://www.redsnapper.net"
"		The governing usage license can be found at http://www.gnu.org/licenses/gpl-3.0.txt"
"		"
"		This program is free software; you can redistribute it and/or modify it"
"		under the terms of the GNU General Public License as published by the Free"
"		Software Foundation; either version 3 of the License, or (at your option)"
"		any later version."
"		"
"		This program is distributed in the hope that it will be useful, but WITHOUT"
"		ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or"
"		FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for"
"		more details."
"		"
"		You should have received a copy of the GNU General Public License along"
"		with this program; if not, write to the Free Software Foundation, Inc.,"
"		59 Temple Place, Suite 330, Boston, MA  02111-1307 USA"
"	</xs:documentation></xs:annotation>"
"	<xs:element name=\"iteration\">"
"		<xs:complexType mixed=\"false\">"
"			<xs:complexContent mixed=\"false\">"
"				<xs:extension base=\"o:flowfunction\">"
"					<xs:sequence>"
"						<xs:element name=\"control\" type=\"o:inputtype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"						<xs:element name=\"body\" type=\"o:inputtype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"					</xs:sequence>"
"					<xs:attribute name=\"operation\" use=\"optional\" default=\"repeat\">"
"						<xs:simpleType>"
"							<xs:restriction base=\"xs:NCName\">"
"								<xs:enumeration value=\"each\"/>"
"								<xs:enumeration value=\"repeat\"/>"
"								<xs:enumeration value=\"sql\"/>"
"								<xs:enumeration value=\"while\"/>"
"								<xs:enumeration value=\"while_not\"/>"
"							</xs:restriction>"
"						</xs:simpleType>"
"					</xs:attribute>"
"				</xs:extension>"
"			</xs:complexContent>"
"		</xs:complexType>"
"	</xs:element>"
"	<xs:element name=\"instruction\">"
"		<xs:complexType mixed=\"false\">"
"			<xs:complexContent mixed=\"false\">"
"				<xs:extension base=\"o:flowfunction\">"
"					<xs:sequence>"
"						<xs:element name=\"input\" type=\"o:inptype\" minOccurs=\"1\" maxOccurs=\"unbounded\" />"
"					</xs:sequence>"
"					<xs:attribute name=\"operation\" use=\"optional\" default=\"assign\">"
"						<xs:simpleType>"
"							<xs:restriction base=\"xs:NCName\">"
"								<xs:enumeration value=\"assign\"/>"
"								<xs:enumeration value=\"add\"/>"
"								<xs:enumeration value=\"append\"/>"
"								<xs:enumeration value=\"arithmetic\"/>"
"								<xs:enumeration value=\"bitwise\"/>"
"								<xs:enumeration value=\"divide\"/>"
"								<xs:enumeration value=\"random\"/>"
"								<xs:enumeration value=\"function\"/>"
"								<xs:enumeration value=\"kind\"/>"
"								<xs:enumeration value=\"left\"/>"
"								<xs:enumeration value=\"length\"/>"
"								<xs:enumeration value=\"lower\"/>"
"								<xs:enumeration value=\"max\"/>"
"								<xs:enumeration value=\"min\"/>"
"								<xs:enumeration value=\"multiply\"/>"
"								<xs:enumeration value=\"position\"/>"
"								<xs:enumeration value=\"query\"/>"
"								<xs:enumeration value=\"quotient\"/>"
"								<xs:enumeration value=\"remainder\"/>"
"								<xs:enumeration value=\"reverse\"/>"
"								<xs:enumeration value=\"right\"/>"
"								<xs:enumeration value=\"shell\"/>"
"								<xs:enumeration value=\"sort\"/>"
"								<xs:enumeration value=\"substring\"/>"
"								<xs:enumeration value=\"subtract\"/>"
"								<xs:enumeration value=\"upper\"/>"
"							</xs:restriction>"
"						</xs:simpleType>"
"					</xs:attribute>"
"					<xs:attribute name=\"precision\" use=\"optional\" type=\"xs:string\" default=\"0\"/>"
"				</xs:extension>"
"			</xs:complexContent>"
"		</xs:complexType>"
"	</xs:element>"
" 	<xs:element name=\"comparison\">"
"		<xs:complexType mixed=\"false\">"
"			<xs:complexContent mixed=\"false\">"
"				<xs:extension base=\"o:flowfunction\">"
"					<xs:sequence>"
"						<xs:element name=\"comparate\" type=\"o:inputtype\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"						<xs:element name=\"ontrue\" type=\"o:inputtype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"						<xs:element name=\"onfalse\" type=\"o:inputtype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"					</xs:sequence>"
"					<xs:attribute name=\"operation\" use=\"optional\" default=\"equal\">"
"						<xs:simpleType>"
"							<xs:restriction base=\"xs:NCName\">"
"								<xs:enumeration value=\"equal\"/>"
"								<xs:enumeration value=\"existent\"/>"
"								<xs:enumeration value=\"empty\"/>"
"								<xs:enumeration value=\"found\"/>"
"								<xs:enumeration value=\"greater\"/>"
"								<xs:enumeration value=\"lesser\"/>"
"								<xs:enumeration value=\"significant\"/>"
"								<xs:enumeration value=\"true\"/>"
"							</xs:restriction>"
"						</xs:simpleType>"
"					</xs:attribute>"
"					<xs:attribute name=\"scope\" use=\"optional\" type=\"o:logic_type\" >"
"						<xs:annotation><xs:documentation>"
"							scope is the legacy name of 'logic', pre. 1.110208"
"							We have to lose the default in order to test for it's existence."
"						</xs:documentation></xs:annotation>"
"					</xs:attribute>"
"					<xs:attribute name=\"logic\" use=\"optional\" default=\"all\" type=\"o:logic_type\" />"
"					<xs:attribute name=\"invert\" use=\"optional\" type=\"xs:boolean\" default=\"false\"/>					"
"				</xs:extension>"
"			</xs:complexContent>"
"		</xs:complexType>"
"	</xs:element>"
"	<xs:element name=\"mapping\">"
"		<xs:complexType mixed=\"false\">"
"			<xs:complexContent mixed=\"false\">"
"				<xs:extension base=\"o:flowfunction\">"
"					<xs:sequence>"
"						<xs:element name=\"domain\" type=\"o:inputtype\" minOccurs=\"1\" maxOccurs=\"1\" />"
"						<xs:element name=\"match\" type=\"o:matchtype\" minOccurs=\"1\" maxOccurs=\"unbounded\" />"
"					</xs:sequence>"
"					<xs:attribute name=\"operation\" use=\"optional\" default=\"switch\">"
"						<xs:simpleType>"
"							<xs:restriction base=\"xs:NCName\">"
"								<xs:enumeration value=\"substitute\"/>"
"								<xs:enumeration value=\"switch\"/>"
"								<xs:enumeration value=\"state\"/>"
"							</xs:restriction>"
"						</xs:simpleType>"
"					</xs:attribute>"
"					<xs:attribute name=\"repeat\" use=\"optional\" type=\"xs:boolean\" default=\"false\"/>"
"				</xs:extension>"
"			</xs:complexContent>"
"		</xs:complexType>"
" 	</xs:element>"
"	<xs:complexType mixed=\"false\" name=\"flowfunction\">"
"		<xs:sequence>"
"			<xs:element name=\"comment\" type=\"o:commenttype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"			<xs:element name=\"output\" type=\"o:outputtype\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"		</xs:sequence>"
"		<xs:attribute name=\"note\" use=\"optional\" type=\"xs:string\"/>"
"		<xs:attribute name=\"version\" use=\"optional\" type=\"xs:float\"/>"
"	</xs:complexType>"
"	<xs:complexType name=\"commenttype\" mixed=\"true\" >"
"		<xs:sequence>"
"			<xs:any namespace=\"##any\" processContents=\"skip\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"		</xs:sequence>"
"	</xs:complexType>"
"	<xs:complexType mixed=\"true\" name=\"inputtype\" >"
"        <xs:sequence>"
"            <xs:element name=\"comment\" type=\"o:commenttype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"            <xs:group ref=\"o:value\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"        </xs:sequence>"
"        <xs:attributeGroup ref=\"o:ikoAttrs\"/>"
"        <xs:attributeGroup ref=\"o:inpAttrs\"/>"
"	</xs:complexType>"
"	<xs:complexType mixed=\"true\" name=\"matchtype\">"
"		<xs:sequence>"
"			<xs:element name=\"comment\" type=\"o:commenttype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"			<xs:element name=\"key\" type=\"o:keytype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"			<xs:group ref=\"o:value\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"		</xs:sequence>"
"		<xs:attributeGroup ref=\"o:ikoAttrs\"/>"
"		<xs:attributeGroup ref=\"o:inpAttrs\"/>"
"	</xs:complexType>"
"	<xs:complexType mixed=\"true\" name=\"inptype\">"
"		<xs:sequence>"
"			<xs:element name=\"comment\" type=\"o:commenttype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"			<xs:group ref=\"o:value\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"		</xs:sequence>"
"		<xs:attributeGroup ref=\"o:ikoAttrs\"/>"
"		<xs:attributeGroup ref=\"o:inpAttrs\"/>"
"		<xs:attribute name=\"name\" use=\"optional\" type=\"xs:NCName\"/>"
"		<xs:attribute name=\"order\" use=\"optional\" default=\"ascending\">"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"ascending\"/>"
"					<xs:enumeration value=\"descending\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>"
"	</xs:complexType>"
"	<xs:complexType mixed=\"true\" name=\"keytype\">"
"		<xs:sequence>"
"			<xs:element name=\"comment\" type=\"o:commenttype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"			<xs:group ref=\"o:value\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"		</xs:sequence>"
"		<xs:attributeGroup ref=\"o:ikoAttrs\"/>"
"		<xs:attributeGroup ref=\"o:inpAttrs\"/>"
"		<xs:attributeGroup ref=\"o:keyAttrs\"/>"
"	</xs:complexType>"
"	<xs:complexType mixed=\"true\" name=\"stype\" >"
"		<xs:complexContent mixed=\"false\">"
"			<xs:restriction base=\"o:inptype\">"
"				<xs:attributeGroup ref=\"o:ikoAttrs\"/>"
"				<xs:attributeGroup ref=\"o:inpAttrs\"/>"
"			</xs:restriction>"
"		</xs:complexContent>"
"	</xs:complexType>"
"	<xs:complexType mixed=\"true\" name=\"outputtype\" >"
"		<xs:sequence>"
"			<xs:element name=\"comment\" type=\"o:commenttype\" minOccurs=\"0\" maxOccurs=\"1\" />"
"			<xs:group ref=\"o:value\" minOccurs=\"0\" maxOccurs=\"unbounded\" />"
"		</xs:sequence>"
"		<xs:attributeGroup ref=\"o:ikoAttrs\"/>"
"		<xs:attributeGroup ref=\"o:outAttrs\"/>"
"	</xs:complexType>"
"	<xs:simpleType name=\"logic_type\">"
"		<xs:restriction base=\"xs:NCName\">"
"			<xs:enumeration value=\"any\"/>"
"			<xs:enumeration value=\"all\"/>"
"		</xs:restriction>"
"	</xs:simpleType>"
"	<xs:simpleType name=\"iko_space_attr\" >"
"		<xs:restriction base=\"xs:NCName\">"
"			<xs:enumeration value=\"namespace\"/>"
"			<xs:enumeration value=\"grammar\"/>"
"			<xs:enumeration value=\"immediate\"/>"
"			<xs:enumeration value=\"file\"/>"
"			<xs:enumeration value=\"cookie\"/>"
"			<xs:enumeration value=\"error\"/>"
"			<xs:enumeration value=\"none\"/>"
"			<xs:enumeration value=\"store\"/>"
"		</xs:restriction>"
"	</xs:simpleType>"
"	<xs:simpleType name=\"out_space_attr\">"
"		<xs:restriction base=\"xs:NCName\">"
"			<xs:enumeration value=\"http\"/>"
"		</xs:restriction>"
"	</xs:simpleType>"
"	<xs:simpleType name=\"inp_space_attr\">"
"		<xs:restriction base=\"xs:NCName\">"
"			<xs:enumeration value=\"url\"/>"
"			<xs:enumeration value=\"field\"/>"
"			<xs:enumeration value=\"sysparm\"/>"
"			<xs:enumeration value=\"sysenv\"/>"
"			<xs:enumeration value=\"parm\"/>"
"		</xs:restriction>"
"	</xs:simpleType>"
"	<xs:group name=\"value\">"
"		<xs:choice >"
"			<xs:element ref=\"o:iteration\"/>"
"			<xs:element ref=\"o:instruction\"/>"
"			<xs:element ref=\"o:comparison\"/>"
"			<xs:element ref=\"o:mapping\"/>"
"			<xs:element name=\"s\" type=\"o:stype\" />"
"			<xs:any namespace=\"##other\" processContents=\"lax\"/>"
"		</xs:choice>"
"	</xs:group>"
"	<xs:attributeGroup name=\"ikoAttrs\">"
"		<xs:attribute name=\"value\" use=\"optional\" type=\"xs:string\"/>"
"		<xs:attribute name=\"xpath\" use=\"optional\" type=\"xs:string\"/>"
"		<xs:attribute name=\"wsstrip\" use=\"optional\" type=\"xs:boolean\" default=\"true\" />"
"		<xs:attribute name=\"kind\" default=\"auto\" >"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"auto\"/>"
"					<xs:enumeration value=\"text\"/>"
"					<xs:enumeration value=\"object\"/>"
"					<xs:enumeration value=\"fragment\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>		"
"		<xs:attribute name=\"context\" use=\"optional\" default=\"none\">"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"none\"/>"
"					<xs:enumeration value=\"store\"/>"
"					<xs:enumeration value=\"file\"/>"
"					<xs:enumeration value=\"cookie\"/>"
"					<xs:enumeration value=\"parm\"/>"
"					<xs:enumeration value=\"field\"/>"
"					<xs:enumeration value=\"url\"/>"
"					<xs:enumeration value=\"sysenv\"/>"
"					<xs:enumeration value=\"sysparm\"/>"
"					<xs:enumeration value=\"namespace\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>"
"		<xs:attribute name=\"encoder\" default=\"none\" >"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"none\"/>"
"					<xs:enumeration value=\"name\"/>"
"					<xs:enumeration value=\"digits\"/>"
"					<xs:enumeration value=\"xml\"/>"
"					<xs:enumeration value=\"url\"/>"
"					<xs:enumeration value=\"sql\"/>"
"					<xs:enumeration value=\"base64\"/>"
"					<xs:enumeration value=\"qp\"/>"
"					<xs:enumeration value=\"hex\"/>"
"					<xs:enumeration value=\"message\"/>"
"					<xs:enumeration value=\"md5\"/>"
"					<xs:enumeration value=\"sha1\"/>"
"					<xs:enumeration value=\"sha512\"/>"
"					<xs:enumeration value=\"deflate\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>		"
"		<xs:attribute name=\"process\" default=\"encode\" >"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"decode\"/>"
"					<xs:enumeration value=\"encode\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>		"
"	</xs:attributeGroup>"
"	<xs:attributeGroup name=\"outAttrs\">"
"		<xs:attribute name=\"space\" use=\"optional\" default=\"immediate\">"
"			<xs:simpleType> "
"				<xs:union memberTypes=\"o:iko_space_attr o:out_space_attr\" />"
"			</xs:simpleType>"
"		</xs:attribute>"
"		<xs:attribute name=\"part\" use=\"optional\" default=\"value\">"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"value\"/>"
"					<xs:enumeration value=\"expires\"/>"
"					<xs:enumeration value=\"path\"/>"
"					<xs:enumeration value=\"domain\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>		"
"		<xs:attribute name=\"scope\" use=\"optional\" default=\"branch\">"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"branch\"/>"
"					<xs:enumeration value=\"global\"/>"
"					<xs:enumeration value=\"ancestor\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>		"
"	</xs:attributeGroup>"
"	<xs:attributeGroup name=\"inpAttrs\">"
"		<xs:attribute name=\"space\" use=\"optional\" default=\"immediate\">"
"			<xs:simpleType> "
"				<xs:union memberTypes=\"o:iko_space_attr o:inp_space_attr\" />"
"			</xs:simpleType>"
"		</xs:attribute>"
"		<xs:attribute name=\"eval\" use=\"optional\" type=\"xs:boolean\" default=\"false\" />"
"		<xs:attribute name=\"release\" use=\"optional\" type=\"xs:boolean\" default=\"false\" />"
"	</xs:attributeGroup>"
"	<xs:attributeGroup name=\"keyAttrs\">"
"		<xs:attribute name=\"break\" use=\"optional\" type=\"xs:boolean\" default=\"false\" />"
"		<xs:attribute name=\"format\" use=\"optional\" default=\"literal\">"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"literal\"/>"
"					<xs:enumeration value=\"regex\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>"
"		<xs:attribute name=\"scope\" use=\"optional\" default=\"all\">"
"			<xs:simpleType>"
"				<xs:restriction base=\"xs:NCName\">"
"					<xs:enumeration value=\"first\"/>"
"					<xs:enumeration value=\"all\"/>"
"				</xs:restriction>"
"			</xs:simpleType>"
"		</xs:attribute>"
"	</xs:attributeGroup>"
"</xs:schema>"
"";
