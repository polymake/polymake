<?xml version="1.0" encoding="ISO-8859-1"?>
	<!--

		this stylesheet transforms the jreality tool config file into
		java.beans.XMLEncoder/XMLDecoder format. See
		de.jreality.scene.tool.config.ToolSystemConfiguration
	-->
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="xml" />

	<xsl:template match="import">
		<inlined>
			<xsl:copy-of select="document(@href)//toolconfig/*" />
		</inlined>
	</xsl:template>

	<xsl:template match="inlined">
		<xsl:apply-templates select="*" />
	</xsl:template>

	<xsl:template match="/toolconfig">
		<java version="1.4.2_08" class="java.beans.XMLDecoder">
			<object class="de.jreality.toolsystem.config.ToolSystemConfiguration">
				<xsl:apply-templates select="*" />
			</object>
		</java>
	</xsl:template>

	<xsl:template match="rawdevices">

		<void property="rawConfigs">
			<xsl:for-each select="rawdevice">
				<void method="add">
					<object class="de.jreality.toolsystem.config.RawDeviceConfig">
						<string>
							<xsl:value-of select="@type" />
						</string>
						<string>
							<xsl:value-of select="@id" />
						</string>
						<object class="java.util.HashMap">
							<xsl:for-each select="prop">
								<void method="put">
									<string>
										<xsl:value-of select="@name" />
									</string>
									<xsl:copy-of select="./*" />
								</void>
							</xsl:for-each>
						</object>
					</object>
				</void>
			</xsl:for-each>
		</void>
	</xsl:template>

	<xsl:template match="rawslots">
		<void property="rawMappings">
			<xsl:for-each select="mapping">
				<void method="add">
					<object class="de.jreality.toolsystem.config.RawMapping">
						<string>
							<xsl:value-of select="@device" />
						</string>
						<string>
							<xsl:value-of select="@src" />
						</string>
						<object class="de.jreality.scene.tool.InputSlot" method="getDevice">
							<string>
								<xsl:value-of select="@target" />
							</string>
						</object>
					</object>
				</void>
			</xsl:for-each>
		</void>

	</xsl:template>

	<xsl:template match="virtualdevices">
		<void property="virtualMappings">
			<xsl:for-each select="mapping">
				<void method="add">
					<object class="de.jreality.toolsystem.config.VirtualMapping">
						<object class="de.jreality.scene.tool.InputSlot" method="getDevice">
							<string>
								<xsl:value-of select="@src" />
							</string>
						</object>
						<object class="de.jreality.scene.tool.InputSlot" method="getDevice">
							<string>
								<xsl:value-of select="@target" />
							</string>
						</object>
					</object>
				</void>
			</xsl:for-each>
		</void>
		<void property="virtualConstants">
			<xsl:for-each select="constant">
				<void method="add">
					<object class="de.jreality.toolsystem.config.VirtualConstant">
						<object class="de.jreality.scene.tool.InputSlot" method="getDevice">
							<string>
								<xsl:value-of select="@name" />
							</string>
						</object>
						<xsl:copy-of select="./*" />
					</object>
				</void>
			</xsl:for-each>
		</void>
		<void property="virtualConfigs">
			<xsl:for-each select="virtualdevice">
				<void method="add">
					<object class="de.jreality.toolsystem.config.VirtualDeviceConfig">
						<string>
							<xsl:value-of select="@type" />
						</string>
						<object class="de.jreality.scene.tool.InputSlot" method="getDevice">
							<string>
								<xsl:value-of select="outputslot" />
							</string>
						</object>
						<object class="java.util.LinkedList">
							<xsl:for-each select="inputslot">
								<void method="add">
									<object class="de.jreality.scene.tool.InputSlot" method="getDevice">
										<string>
											<xsl:value-of select="." />
										</string>
									</object>
								</void>
							</xsl:for-each>
						</object>
						<object class="java.util.HashMap">
							<xsl:for-each select="prop">
								<void method="put">
									<string>
										<xsl:value-of select="@name" />
									</string>
									<xsl:copy-of select="./*" />
								</void>
							</xsl:for-each>
						</object>
						<string>
							<xsl:value-of select="mapping" />
						</string>
					</object>
				</void>
			</xsl:for-each>
		</void>
	</xsl:template>

	<xsl:template match="/java/object">
		<java version="1.4.2_08" class="java.beans.XMLDecoder">
			<object class="de.jreality.toolsystem.config.ToolSystemConfiguration">
				<xsl:apply-templates select="*" />
			</object>
		</java>
	</xsl:template>

	<xsl:template match="void">
		<xsl:copy-of select="." />
	</xsl:template>
</xsl:stylesheet>
