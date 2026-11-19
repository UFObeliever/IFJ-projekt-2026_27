import ifj26.*

def greet(name: String): String = s"Hi $name"

@main
def main(): Unit =
  val g = greet("Bob")
  print(g)
