using Microsoft.OpenApi.Models;
using Images.Models;
using Microsoft.EntityFrameworkCore;

var builder = WebApplication.CreateBuilder(args);

// Set up Swagger 
builder.Services.AddEndpointsApiExplorer();
// builder.Services.AddDbContext<ImageDb>(options => options.UseInMemoryDatabase("items"));
builder.Services.AddDbContext<ImageDb>(opt => 
    opt.UseNpgsql(builder.Configuration.GetConnectionString("GestureDrawingConnection")));

builder.Services.AddSwaggerGen(c =>
{
     c.SwaggerDoc("v1", new OpenApiInfo { Title = "Images API", Description = "Storing Images for Gesture Drawing App", Version = "v1" });
});

var app = builder.Build();

//Swagger UI
if (app.Environment.IsDevelopment())
{ 
    app.UseSwagger();
    app.UseSwaggerUI(c =>
    {
        c.SwaggerEndpoint("/swagger/v1/swagger.json", "GestureDrawing API V1");
    });
}



// app.MapGet("/", () => "Hello World!");
app.MapGet("/image", async (ImageDb db) => await db.Images.ToListAsync());

// Post 
app.MapPost("/image", async (ImageDb db, Image image) => {
    // call the dbset that is inside the db 
    // AddAsync() adds image to the dbset
    // using await is important here for both calls as we need the async call back to ensure the actions have been completed
    await db.Images.AddAsync(image);
    
    //after adding the image, we need to save the changes
    await db.SaveChangesAsync();

    return Results.Created($"/image/{image.id}", image); // have to do "Id" with a capital "I" because that is the field we declared inside Image.cs for the class Image 
});

// Delete route
app.MapDelete("/image/{id}", async(ImageDb db, int id) => {
    // Find image with an the id given in route \
    // IMPORTANT: need to use await keyword here 
    var image = await db.Images.FindAsync(id);

    //If there is no image with an id in the given route, return that the result was not found
    if (image is null) return Results.NotFound();

    //if the image is present, remove it
    //IMPORTANT: need to use Remove() not remove()
    db.Images.Remove(image);

    //Save changed db after removing image with id given in route
    await db.SaveChangesAsync();

    //return ok result to indicate the removal was complete
    return Results.Ok();
});

app.Run();