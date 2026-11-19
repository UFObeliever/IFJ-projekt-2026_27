import ifj26.*

def maybe2(x: Int) =
  if x > 0 then
    print("p")

@main
def main(): Unit =
  maybe2(1)
  maybe2(-1)
