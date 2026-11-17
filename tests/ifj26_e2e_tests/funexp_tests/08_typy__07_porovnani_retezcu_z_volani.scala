import ifj26.*

def s2(x: String): String = x + x

@main
def main(): Unit =
  if s2("a") < s2("b") then
    print("y")
  else
    print("n")
