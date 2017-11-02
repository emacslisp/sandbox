import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.File;

public class CommandLineUtil {
    public static void main(String[] args)
    {
        // TODO Auto-generated method stub
        System.out.println(runCommand("g++ -static /home/ubuntu/code/sandbox/create/vectorTest.cpp -o /home/ubuntu/code/sandbox/create/vectorTest"));
        System.out.println(runCommand("/home/ubuntu/code/sandbox/create/oj.sh /home/ubuntu/code/sandbox/create/vectorTest"));
        //        System.out.println(runCommand("gcc -static /home/ubuntu/code/sandbox/create/test.c -o /home/ubuntu/code/sandbox/create/test"));                
        //        System.out.println(runCommand("/home/ubuntu/code/sandbox/sample2 /home/ubuntu/code/sandbox/create/test"));        
        //compare file one line by one line, and put diff
        // if everything is good, then return AC
        //System.out.println("test");
    }

    public static String runCommand(String input) {
        String result="";

        try {
            Process process = Runtime.getRuntime().exec(input);
            try {
                process.waitFor();
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            
            BufferedReader read = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String str = read.readLine();
            while (str != null){
                System.out.println(str);
                str = read.readLine();
                result += str;
                result += "\n";
            }
            
            InputStream error = process.getErrorStream();
            InputStreamReader isrerror = new InputStreamReader(error);
            BufferedReader bre = new BufferedReader(isrerror);
            while ((str = bre.readLine()) != null) {
                result += str;
                result += "\n";
            }

            
            
            result += "exit value is " + process.exitValue();

        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        return result;
    }
}
    
