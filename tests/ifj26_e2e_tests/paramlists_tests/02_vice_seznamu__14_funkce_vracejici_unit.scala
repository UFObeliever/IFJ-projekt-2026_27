import ifj26.*

def say(a: String)(b: String): Unit =
  print(a)
  print(b)

@main
def main(): Unit =
  say("x")("y")
