using Microsoft.EntityFrameworkCore;

namespace Images.Models 
{
    public class Image 
    {
        public int id {get; set;}
        public string? category {get; set;}
        public string? imagebinary {get; set;}
    }
    // to create a migration (adding new fields or changing name!): dotnet ef migrations add infoWord. infoword is similar to when you make a comment on git commit!
    // Note to self: when inserting values into postgresql table, if the value is a string, need to use single quotes and not double quotes to input it!
    class ImageDb : DbContext
    {
        public ImageDb(DbContextOptions options) : base(options) {}
        public DbSet<Image> Images {get; set;} = null!;

        protected override void OnModelCreating(ModelBuilder builder) 
        {
            builder.Entity<Image>().ToTable("images");
        }
    }
}
