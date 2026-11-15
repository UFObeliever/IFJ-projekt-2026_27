import ifj26.*

def greet(name: String): String = "Hello, " + name

@main def main(): Unit =
  val g = greet("IFJ")
  print(g)
  print("\n")
