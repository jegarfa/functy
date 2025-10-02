<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:wi="http://schemas.microsoft.com/wix/2006/wi">
  <xsl:output method="xml" indent="no" omit-xml-declaration="yes"/>
  <!-- <xsl:strip-space elements="*"/> -->
  <xsl:key name="kCompsToRemove" match="wi:Component[contains(wi:File/@Source, 'Makefile.am')]" use="@Id" />

  <xsl:template match="@* | node()">
    <xsl:copy>
      <xsl:apply-templates select="@* | node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="*[self::wi:Component or self::wi:ComponentRef] [key('kCompsToRemove', @Id)]" />
</xsl:stylesheet>