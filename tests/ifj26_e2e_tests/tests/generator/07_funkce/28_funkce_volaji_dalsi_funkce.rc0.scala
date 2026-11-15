import ifj26.*

def c(): Unit = print("c")

def b(): Unit =
  print("b")
  c()
  print("b")

def a(): Unit =
  print("a")
  b()
  print("a")

@main def main(): Unit =
  a()
  print("\n")
