using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace backend.Migrations
{
    /// <inheritdoc />
    public partial class NameChanged : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "ImageBinary",
                table: "Images",
                newName: "imageBinary");

            migrationBuilder.RenameColumn(
                name: "Id",
                table: "Images",
                newName: "id");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.RenameColumn(
                name: "imageBinary",
                table: "Images",
                newName: "ImageBinary");

            migrationBuilder.RenameColumn(
                name: "id",
                table: "Images",
                newName: "Id");
        }
    }
}
